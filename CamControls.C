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

#include "CamControls.H"
#include "Serial.H"
#include "SpinSlider.H"

#include <QGridLayout>
#include <QRegularExpression>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>

// ##############################################################################################################
CamControls::CamControls(Serial * serport, QWidget * parent) :
    QWidget(parent), m_serial(serport), m_timer(this)
{
  // Setup our timer to refresh the widgets once in a while:
  connect(&m_timer, &QTimer::timeout,
	  [this]()
	  {
	    m_serial->command("caminfo", [this](QStringList const & controls) { this->refresh(controls); });
	  } );
  DEBU("Ready");
}

// ##############################################################################################################
CamControls::~CamControls()
{
  m_timer.stop();
}

// ##############################################################################################################
void CamControls::tabselected()
{
  m_serial->command("caminfo", [this](QStringList const & controls) { this->build(controls); });
  m_timer.start(std::chrono::milliseconds(1300));
}

// ##############################################################################################################
void CamControls::tabunselected()
{
  m_timer.stop();
}

// ##############################################################################################################
void CamControls::build(QStringList const & controls)
{
  m_ctrl.clear();
  
  // Nuke all widgets and the layout if needed:
  if (m_table.data())
  {
    while (QLayoutItem * item = m_table->takeAt(0))
    {
      if (QWidget * widget = item->widget()) { widget->hide(); delete widget; }
      else delete item;
    }
    m_table.reset();
  }
  
  // Now create a new layout with this as parent:
  m_table.reset(new QGridLayout(this));

#ifdef Q_OS_MACOS
  m_table->setSpacing(2);
  m_table->setMargin(2);
#endif
  
  int idx = 0;
  
  for (auto const & str : controls)
  {
    QStringList vec = str.split(QRegularExpression("\\s+"));

    if (vec[0] == "OK") continue;

    QLabel * controlName = new QLabel(vec[0]);
    m_table->addWidget(controlName, idx, 0);
    m_table->setAlignment(controlName, Qt::AlignVCenter);
    
    // Handle dowb with a pushbutton:
    if (vec[0] == "dowb") // name B def curr
    {
      auto chk = new QPushButton(tr("Compute White Balance Now"));
      connect(chk, &QPushButton::clicked, [this](bool ) { m_serial->setCam("dowb", 1); });
      m_table->addWidget(chk, idx, 1);
      m_ctrl[vec[0]] = { chk, [](int) { /* oneshot does not get updated */ } };
    }
    
    // Handle integer widget with a slider:
    else if (vec[1] == "I") // name I min max step def curr
    {
      auto slider = new SpinSlider(vec[2].toInt(), vec[3].toInt(), vec[4].toInt(), Qt::Horizontal);
      slider->setFocusPolicy(Qt::StrongFocus); // do not move the slider while mouse wheel scrolling
      QString const name = vec[0];
      int const defval = vec[5].toInt();
      slider->setValue(vec[6].toInt());
      
      connect(slider, &SpinSlider::valueChanged, [this, name, idx](int val)
              {
                SpinSlider * sli = dynamic_cast<SpinSlider *>(m_table->itemAtPosition(idx, 1)->widget());
                if (sli) m_serial->setCam(name, val);
                else DEBU("ooops camera param " << name << " is not a spinslider");
              });
      m_table->addWidget(slider, idx, 1);
      m_ctrl[name] = { slider, [slider](int val) { if (val != slider->value()) slider->setValue(val); } };
      
      auto button = new QPushButton("Reset");
      connect(button, &QPushButton::clicked,
              [this, idx, name, defval]()
              {
                SpinSlider * sli = dynamic_cast<SpinSlider *>(m_table->itemAtPosition(idx, 1)->widget());
                if (sli && sli->isEnabled()) sli->setValue(defval); // will also update the hardware
                else DEBU("ooops camera param " << name << " is not a spinslider or disabled"); 
              });
      
      m_table->addWidget(button, idx, 2);
    }
    
    // Handle a boolean with a checkbox:
    else if (vec[1] == "B") // name B def curr
    {
      auto chk = new QCheckBox();
      QString const name = vec[0];
      int const defval = vec[2].toInt();
      chk->setCheckState(vec[3].toInt() ? Qt::Checked : Qt::Unchecked);
      auto callback = (name == "autowb" || name == "autogain") ?
        [&](QStringList const &) { updateDependencies(); } : std::function<void(QStringList const &)>();
      
      connect(chk, &QCheckBox::stateChanged, [this, name, idx, callback](int val)
              {
                QCheckBox * cb = dynamic_cast<QCheckBox *>(m_table->itemAtPosition(idx, 1)->widget());
                if (cb) m_serial->setCam(name, val == Qt::Checked ? "1" : "0", callback);
                else DEBU("ooops camera param " << name << " is not a checkbox");
              });
      m_table->addWidget(chk, idx, 1);
      m_ctrl[name] = { chk, [chk](int val) {
          if (val && chk->checkState() == Qt::Unchecked) chk->setCheckState(Qt::Checked);
          else if (val == 0 && chk->checkState() == Qt::Checked) chk->setCheckState(Qt::Unchecked); } };
      
      auto button = new QPushButton("Reset");
      connect(button, &QPushButton::clicked,
              [this, idx, name, defval]()
              {
                QCheckBox * cb = dynamic_cast<QCheckBox *>(m_table->itemAtPosition(idx, 1)->widget());
                if (cb && cb->isEnabled()) cb->setCheckState(defval ? Qt::Checked : Qt::Unchecked); // will upd hardware
                else DEBU("ooops camera param " << name << " is not a checkbox or disabled"); 
              });
      
      m_table->addWidget(button, idx, 2);
    }
    
    // Handle a menu item
    else if (vec[1] == "M") // name M def curr 1:name1 2:name2 ...
    {
      auto cbox = new QComboBox();
      cbox->setFocusPolicy(Qt::StrongFocus); // do not spin the box while mouse wheel scrolling
      QString const name = vec[0];
      int const defval = vec[2].toInt();
      int const currval = vec[3].toInt();
      int defindex = 0;
      
      int midx = 0;
      for (int i = 4; i < vec.size(); ++i)
      {
        QStringList const vv = vec[i].split(':');
        if (vv.size() != 2) break;
        int const val = vv[0].toInt();
        cbox->addItem(vv[1], QVariant(val));
        if (val == currval) cbox->setCurrentIndex(midx);
        if (val == defval) defindex = midx;
        ++ midx;
      }
      
      auto callback = (name == "autoexp") ?
        [&](QStringList const &) { updateDependencies(); } : std::function<void(QStringList const &)>();
      
      connect(cbox, QOverload<int>::of(&QComboBox::currentIndexChanged),
              [this, name, idx, callback](int index)
              {
                QComboBox * cb = dynamic_cast<QComboBox *>(m_table->itemAtPosition(idx, 1)->widget());
                if (cb) m_serial->setCam(name, QString::number(cb->itemData(index).toInt()), callback);
                else DEBU("ooops camera param " << name << " is not a combo box");
              });
      
      m_table->addWidget(cbox, idx, 1);
      m_ctrl[name] = { cbox, [cbox](int val) { if (val != cbox->currentIndex()) cbox->setCurrentIndex(val); } };
      
      auto button = new QPushButton("Reset");
      connect(button, &QPushButton::clicked,
              [this, idx, name, defindex]()
              {
                QComboBox * cb = dynamic_cast<QComboBox *>(m_table->itemAtPosition(idx, 1)->widget());
                if (cb && cb->isEnabled()) cb->setCurrentIndex(defindex); // will also update the hardware
                else DEBU("ooops camera param " << name << " is not a combo box"); 
              });
      
      m_table->addWidget(button, idx, 2);
    }
    
    ++idx;
  }
  
  // Add some stretch at the bottom:
  m_table->addWidget(new QLabel("  "), idx, 0, 1, 3);
  m_table->setRowStretch(idx++, 20);
  
  // Grey out disabled controls:
  updateDependencies();

  // On mac, we need this for the widgets to draw correctly:
  update();
}

// ##############################################################################################################
void CamControls::refresh(QStringList const & controls)
{
  for (auto const & str : controls)
  {
    QStringList vec = str.split(QRegularExpression("\\s+"));

    if (vec[0] == "OK") continue;

    // Get the control:
    auto itr = m_ctrl.find(vec[0]);
    if (itr == m_ctrl.end()) continue; // We don't have this control
    
    int val;
    if (vec[1] == "I") val = vec[6].toInt();  // name I min max step def curr
    else if (vec[1] == "B") val = vec[3].toInt(); // name B def curr
    else if (vec[1] == "M") val = vec[3].toInt(); // name M def curr 1:name1 2:name2 ...
    else { DEBU("Camera control of unknown type " << vec[1] << " ignored"); continue; }

    // All right, just set the value:
    itr->second.set(val);
  }
  
  // Grey out disabled controls:
  updateDependencies();
}

// ##############################################################################################################
void CamControls::updateDependencies()
{
  // redbal and bluebal depend on autowb:
  auto awb = dynamic_cast<QCheckBox *>(m_ctrl["autowb"].w);
  if (awb)
  {
    bool on = (awb->checkState() == Qt::Checked);
    auto rb = m_ctrl["redbal"].w; if (rb) { if (on) rb->setEnabled(false); else rb->setEnabled(true); }
    auto bb = m_ctrl["bluebal"].w; if (bb) { if (on) bb->setEnabled(false); else bb->setEnabled(true); }
  }

  // gain depends on autogain:
  auto ag = dynamic_cast<QCheckBox *>(m_ctrl["autogain"].w);
  if (ag)
  {
    bool on = (ag->checkState() == Qt::Checked);
    auto g = m_ctrl["gain"].w; if (g) { if (on) g->setEnabled(false); else g->setEnabled(true); }
  }

  // absexp depends on autoexp.
  // Also note (seems to be working fine as it is):
  // autoexp to manual triggers autogain to manual and hence gain enabled
  // autoexp to auto triggers autogain to auto and hence gain disabled
  auto ae = dynamic_cast<QComboBox *>(m_ctrl["autoexp"].w);
  if (ae)
  {
    bool on = (ae->currentText() == "auto");
    auto e = m_ctrl["absexp"].w; if (e) { if (on) e->setEnabled(false); else e->setEnabled(true); }
  }
}
