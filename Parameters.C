// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2018 by Laurent Itti, the University of Southern
// California (USC), and iLab at USC. See http://iLab.usc.edu and http://jevois.org for information about this project.
//
// This file is part of the JeVois Smart Embedded Machine Vision Toolkit.  This program is free software; you can
// redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
// Foundation, version 2.  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
// License for more details.  You should have received a copy of the GNU General Public License along with this program;
// if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// Contact information: Laurent Itti - 3641 Watt Way, HNB-07A - Los Angeles, CA 90089-2520 - USA.
// Tel: +1 213 740 3527 - itti@pollux.usc.edu - http://iLab.usc.edu - http://jevois.org
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "Parameters.H"
#include "Serial.H"
#include "ParamInfo.H"
#include "Utils.H"
#include "SpinRangeSlider.H"
#include "SpinSlider.H"

#include <QGridLayout>
#include <QRegularExpression>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QScrollArea>
#include <QLineEdit>
#include <QMessageBox>

// ##############################################################################################################
Parameters::Parameters(Serial * serport, QWidget * parent) :
    QWidget(parent),
    m_serial(serport),
    m_fbutton(tr("Show Frozen Parameters")),
    m_sbutton(tr("Show System Parameters")),
    m_scroll(),
    m_widget(nullptr),
    m_built(false)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  auto lay = new QVBoxLayout(this);
  lay->setMargin(3); lay->setSpacing(3); // using 0 messes up the layout on mac

  // First add the buttons for frozen, module only:
  auto hlay = new QHBoxLayout();
  hlay->setMargin(JVINV_MARGIN); hlay->setSpacing(3);
  
  m_fbutton.setCheckable(true);
  hlay->addWidget(&m_fbutton);
  connect(&m_fbutton, &QPushButton::toggled, [this](bool) { tabselected(); });

  hlay->addStretch(10);
  
  m_sbutton.setCheckable(true);
  hlay->addWidget(&m_sbutton);
  connect(&m_sbutton, &QPushButton::toggled, [this](bool) { tabselected(); });

  lay->addLayout(hlay);

  // Now a scroll area for our parameters:
  lay->addWidget(&m_scroll);
  m_scroll.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_scroll.setWidgetResizable(true);
#ifdef Q_OS_MACOS
  // Getting some weird redraw errors on macs unless this is set...
  m_scroll.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
#endif
}

// ##############################################################################################################
Parameters::~Parameters()
{ }

// ##############################################################################################################
void Parameters::tabselected()
{
  QString cmd = "paraminfo";
  bool const showFrozen = m_fbutton.isChecked();
  bool const showSystem = m_sbutton.isChecked();
  if (showFrozen == false && showSystem == false) cmd += " modhot";
  else if (showFrozen == false && showSystem) cmd += " hot";
  else if (showFrozen && showSystem == false) cmd += " mod";
  
  m_serial->command(cmd, [this](QStringList const & controls) { this->build(controls); });
}

// ##############################################################################################################
void Parameters::build(QStringList const & params)
{
  // Indicate that we are going down to prevent some signals/slots from crashing us...
  m_built = false;

  // Nuke all widgets and the layout if needed:
  if (m_table.data())
  {
    while (QLayoutItem * item = m_table->takeAt(0))
    {
      if (QWidget * widget = item->widget())
      { DEBU("del widget "<<widget); widget->hide(); delete widget; delete item; }
      else
      { DEBU("del " <<item); delete item; }
    }
    m_table.reset();
  }
  if (m_widget) delete m_widget;
  
  // Now create a new grid layout for the params:
  std::map<QString /* category */, std::map<QString /* descriptor */, ParamInfo> > pmap = parseParamInfo(params);
  m_widget = new QWidget(this);
  m_table.reset(new QGridLayout(m_widget));
  m_table->setMargin(3 /*JVINV_MARGIN*/); m_table->setSpacing(3/*JVINV_SPACING*/);
  m_table->setSizeConstraint(QLayout::SetMinAndMaxSize);

  // Now create the widgets:
  int idx = 0; // this is now the row index in our table
  for (auto const & pc : pmap)
  {
    // Create a header row for that category:
    QLabel * header = new QLabel(pc.first);
    header->setStyleSheet("QLabel { background-color: lightgray; color: darkblue; }");
    header->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    header->setMargin(5);
    m_table->addWidget(header, idx, 0, 1, 3);
    ++idx;
    
    // Now insert all the widgets for that category:
    for (auto const & pd : pc.second)
    {
      ParamInfo const & p = pd.second;
      QString const & defval = p.defaultvalue;
      QString const & descrip = pd.first;
      int defindex = 0; // Used by combobox only
      QString const & vtype = p.valuetype;
      
      bool const is_int = (vtype == "short" || vtype == "int" || vtype == "long int" || vtype == "long long int");
      bool const is_uint = (vtype == "unsigned char" || vtype == "unsigned short" || vtype == "unsigned int" || 
                            vtype == "unsigned long int" || vtype == "unsigned long long int" || vtype == "size_t");
      bool const is_real = (vtype == "float" || vtype == "double" || vtype == "long double");
      //bool const is_number = (is_int || is_uint || is_real);
      
      // Parameter name with descriptor and value type as a tooltip:
      QLabel * lbl = new QLabel(p.displayname);
      lbl->setToolTip(descrip + " [" + p.valuetype + ']');
      m_table->addWidget(lbl, idx, 0);
      m_table->setAlignment(lbl, Qt::AlignVCenter);
      
      // Widget with description as tooltip:
      // ----------------------------------------------------------------------
      QWidget * widget;
      if (p.validvalues.startsWith("List:["))
      {
        auto wi = new QComboBox; widget = wi;
        wi->setFocusPolicy(Qt::StrongFocus); // do not spin the box while mouse wheel scrolling
        QString vals = p.validvalues.mid(6, p.validvalues.size() - 7);
        
        QStringList vv = vals.split('|');
        for (QString const & v : vv)
        {
          wi->addItem(v);
          if (v == p.value) wi->setCurrentIndex(wi->count() - 1);
          if (v == p.defaultvalue) defindex = wi->count() - 1;
        }

        connect(wi, QOverload<int>::of(&QComboBox::currentIndexChanged),  [this, descrip, idx]() {
            if (m_built == false) return; // Prevent crash during destruction of the param table
            auto ww = dynamic_cast<QComboBox *>(m_table->itemAtPosition(idx, 1)->widget());
            if (ww)
            {
              QString const val = ww->currentText();
              m_serial->
                setPar(descrip, val,
                       [this](QStringList const &) { setParOk(); },
                       [this, descrip, val](QStringList const & data) { setParErr(descrip, val, data);} );
            }
          });
      }
      // ----------------------------------------------------------------------
      else if (vtype == "unsigned char" ||
               (p.validvalues.startsWith("Range:[") && (is_int || is_uint)))
      {
        // Parse spec of the form: Range:[0.01 ... 1000]
        QStringList vec = p.validvalues.split(QRegularExpression(R"(\[|\]|\.\.\.)"));
        int mini = 0, maxi = 255;
        if (vec.size() == 4) { mini = vec[1].toInt(); maxi = vec[2].toInt(); }
        else DEBU("ooops "<<descrip<<" vec is:" << vec);
        
        auto wi = new SpinSlider(mini, maxi, 1, Qt::Horizontal); widget = wi;
        wi->setFocusPolicy(Qt::StrongFocus); // do not slide while mouse wheel scrolling
        wi->setValue(p.value.toInt());
        
        connect(wi, &SpinSlider::valueChanged, [this, descrip, idx](int valint) {
            if (m_built == false) return; // Prevent crash during destruction of the param table
            auto ww = dynamic_cast<SpinSlider *>(m_table->itemAtPosition(idx, 1)->widget());
            if (ww)
            {
              QString const val = QString::number(valint);
              m_serial->
                setPar(descrip, val,
                       [this](QStringList const &) { setParOk(); },
                       [this, descrip, val](QStringList const & data) { setParErr(descrip, val, data); } );
            }
          });
      }
      // ----------------------------------------------------------------------
      else if (vtype == "jevois::Range<unsigned char>")
      {
        auto wi = new SpinRangeSlider(0, 255); widget = wi;
        wi->setFocusPolicy(Qt::StrongFocus); // do not slide while mouse wheel scrolling
        int lower = 0, upper = 255;
        QStringList vec = p.value.split(QRegularExpression(R"(\.\.\.)"));
        if (vec.size() == 2) { lower = vec[0].toInt(); upper = vec[1].toInt(); }
        wi->setLowerValue(lower);
        wi->setUpperValue(upper);
        
        connect(wi, &SpinRangeSlider::lowerValueChanged, [this, descrip, idx](int lower) {
            if (m_built == false) return; // Prevent crash during destruction of the param table
            auto ww = dynamic_cast<SpinRangeSlider *>(m_table->itemAtPosition(idx, 1)->widget());
            if (ww)
            {
              QString const val = QString::number(lower) + " ... " + QString::number(ww->GetUpperValue());
              m_serial->
                setPar(descrip, val,
                       [this](QStringList const &) { setParOk(); },
                       [this, descrip, val](QStringList const & data) { setParErr(descrip, val, data); } );
            }
          });
        
        connect(wi, &SpinRangeSlider::upperValueChanged, [this, descrip, idx](int upper) {
            if (m_built == false) return; // Prevent crash during destruction of the param table
            auto ww = dynamic_cast<SpinRangeSlider *>(m_table->itemAtPosition(idx, 1)->widget());
            if (ww)
            {
              QString const val = QString::number(ww->GetLowerValue()) + " ... " + QString::number(upper);
              m_serial->
                setPar(descrip, val,
                       [this](QStringList const &) { setParOk(); },
                       [this, descrip, val](QStringList const & data) { setParErr(descrip, val, data); } );
            }
          });
      }
      // ----------------------------------------------------------------------
      else if (p.valuetype == "bool")
      {
        auto wi = new QCheckBox; widget = wi;
        wi->setCheckState(p.value == "true" ? Qt::Checked : Qt::Unchecked);
        
        connect(wi, &QCheckBox::stateChanged, [this, descrip, idx](int value) {
            if (m_built == false) return; // Prevent crash during destruction of the param table
            auto ww = dynamic_cast<QCheckBox *>(m_table->itemAtPosition(idx, 1)->widget());
            if (ww)
            {
              QString val = (value == Qt::Checked ? "true" : "false");
              m_serial->
                setPar(descrip, val,
                       [this](QStringList const &) { setParOk(); },
                       [this, descrip, val](QStringList const & data) { setParErr(descrip, val, data); } );
            }
          });
        
      }
      // ----------------------------------------------------------------------
      else
      {
        // User will type in some value:
        auto wi = new QLineEdit; widget = wi;
        wi->setText(p.value);
        if (is_int || is_uint) wi->setValidator(new QIntValidator(wi));
        else if (is_real) wi->setValidator(new QDoubleValidator(wi));
        
        connect(wi, &QLineEdit::editingFinished, [this, descrip, idx]() {
            if (m_built == false) return; // Prevent crash during destruction of the param table
            auto ww = dynamic_cast<QLineEdit *>(m_table->itemAtPosition(idx, 1)->widget());
            if (ww)
            {
              if (ww->isModified() == false) return; // do not setpar on no-op edits
              QString const val = ww->text();
              m_serial->
                setPar(descrip, val,
                       [this](QStringList const &) { setParOk(); },
                       [this, descrip, val](QStringList const & data) { setParErr(descrip, val, data); } );
              ww->setModified(false); // Avoid double hit when we remove focus
              setFocus(); // steal the focus from the param so we know editing is over
            }
          });
      }
      
      // Finalize and insert the widget into the table:
      widget->setToolTip(splitToolTip(p.description));
      if (p.frozen) widget->setDisabled(true);
      m_table->addWidget(widget, idx, 1);
      
      // A reset button:
      auto button = new QPushButton("Reset");
      connect(button, &QPushButton::clicked,
              [this, idx, defval, defindex]()
              {
                if (m_built == false) return; // Prevent crash during destruction of the param table
                
                auto le = dynamic_cast<QLineEdit *>(m_table->itemAtPosition(idx, 1)->widget());
                if (le) { le->setText(defval); return; } // will also update the hardware
                
                auto cb = dynamic_cast<QComboBox *>(m_table->itemAtPosition(idx, 1)->widget());
                if (cb) { cb->setCurrentIndex(defindex); return; } // will also update the hardware
                
                auto ck = dynamic_cast<QCheckBox *>(m_table->itemAtPosition(idx, 1)->widget());
                if (ck) { ck->setCheckState(defval == "true" ? Qt::Checked : Qt::Unchecked); return; }
                
                auto sl = dynamic_cast<SpinSlider *>(m_table->itemAtPosition(idx, 1)->widget());
                if (sl) { sl->setValue(defval.toInt()); return; }
                
                auto rs = dynamic_cast<SpinRangeSlider *>(m_table->itemAtPosition(idx, 1)->widget());
                if (rs) {
                  int lower = 0, upper = 255;
                  QStringList vec = defval.split(QRegularExpression(R"(\.\.\.)"));
                  if (vec.size() == 2) { lower = vec[0].toInt(); upper = vec[1].toInt(); }
                  rs->setLowerValue(lower);
                  rs->setUpperValue(upper);
                  return;
                }
                
              });
      
      if (p.frozen) button->setDisabled(true);
      m_table->addWidget(button, idx, 2);
      
      // Done with this parameter:
      ++idx;
    }
  }

  if (idx == 0) m_table->addWidget(new QLabel(tr("This module has no parameters.")), idx++, 0, 1, 3);

  // Add some stretch at the bottom:
  m_table->addWidget(new QLabel("  "), idx, 0, 1, 3);
  m_table->setRowStretch(idx++, 20);

  // Finalize:
  m_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_widget->setMinimumWidth(size().width() - 20);
  m_scroll.setWidget(m_widget);

  // On mac, we need this for the widgets to draw correctly:
  update();
  m_built = true;
}

// ##############################################################################################################
void Parameters::setParOk()
{ }

// ##############################################################################################################
void Parameters::setParErr(QString const & descrip, QString const & value, QStringList const & data)
{
  for (QString const & s : data)
  {
    if (s == "OK") return;
    if (s.startsWith("ERR "))
    {
      QMessageBox::warning(this, "Setting parameter failed", "Parameter [" + descrip + "] change to [" + value +
                           "] rejected with reason:\n\n" + s);
      return;
    }
  }
}
