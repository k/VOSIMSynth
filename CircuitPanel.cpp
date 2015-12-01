#include "CircuitPanel.h"
#include "UI.h"
#include "VOSIMSynth.h"
#include "mutex.h"

namespace syn
{
  UnitControl::UnitControl(IPlugBase* pPlug, VoiceManager* vm, Unit* unit, int x, int y, int size) :
    m_size(size),
    m_x(x),
    m_y(y),
    m_unit(unit),
    m_nParams(unit->getParameterNames().size()),
    m_is_sink(false),
    IControl(pPlug, { x,y,x + size,y + size })
  {
    for (int i = 0; i < m_nParams; i++)
    {
      m_portLabels.push_back(ITextSlider(pPlug, vm, IRECT{ 0,0,0,0 }, unit->getParent().getUnitId(unit), i));
    }
    m_ports.resize(m_nParams);
    resize(size);
  }

  UnitControl::~UnitControl()
  {}

  void UnitControl::move(int newx, int newy)
  {
    if (newx < 0 || newy < 0) return;
    m_x = newx;
    m_y = newy;
    mRECT.L = newx;
    mRECT.T = newy;
    mRECT.R = newx + m_size;
    mRECT.B = newy + m_size;
    SetTargetArea(mRECT);
    resize(m_size);
  }

  void UnitControl::resize(int newsize)
  {
    const vector<string>& paramNames = m_unit->getParameterNames();
    if (newsize <= 10 * m_nParams + 30) newsize = m_nParams * 10 + 30;
    m_size = newsize;
    mRECT.R = m_x + m_size;
    mRECT.B = m_y + m_size;

    int portY = m_y + 20;
    int rowsize = (newsize - 20) / (double)m_nParams;
    for (int i = 0; i < paramNames.size(); i++)
    {
      m_portLabels[i].setRect(IRECT{ m_x + 30 ,portY, m_x + newsize,portY + 10 });
      m_ports[i].add_rect = IRECT{ m_x, portY, m_x + 10, portY + 10 };
      m_ports[i].scale_rect = IRECT{ m_x + 12, portY, m_x + 22, portY + 10 };
      portY += rowsize;
    }
    SetTargetArea(mRECT);
  }

  bool UnitControl::Draw(IGraphics* pGraphics)
  {
    Instrument* instr = (Instrument*)&m_unit->getParent();
    IText textfmt{ 12,0,"Helvetica",IText::kStyleNormal,IText::kAlignNear,0,IText::kQualityClearType };
    IText textfmtwhite{ 12,&IColor{ 255,255,255,255 },"Helvetica",IText::kStyleNormal,IText::kAlignNear,0,IText::kQualityClearType };
    IText centertextfmt{ 12,0,"Helvetica",IText::kStyleNormal,IText::kAlignCenter,0,IText::kQualityClearType };
    // If this unit is the circuit sink
    if (instr->getSinkId() == instr->getUnitId(m_unit))
    {
      pGraphics->FillIRect(&(IColor)palette[1], &mRECT);
    }
    else
    {
      pGraphics->FillIRect(&(IColor)palette[2], &mRECT);
    }

    if (instr->isPrimarySource(instr->getUnitId(m_unit)))
    {
      pGraphics->DrawRect(&IColor{ 255,255,255,255 }, &(mRECT.GetPadded(10)));
    }

    VOSIMSynth* vs = (VOSIMSynth*)mPlug;
    // If this unit is the oscilloscope input
    if (vs->m_oscilloscope_input_unit == m_unit->getParent().getUnitId(m_unit))
    {
      pGraphics->DrawIText(&textfmt, "I", &IRECT{ mRECT.R - 10,mRECT.B - 10,mRECT.R,mRECT.B });
    }
    if (vs->m_oscilloscope_trigger_unit == m_unit->getParent().getUnitId(m_unit))
    {
      pGraphics->DrawIText(&textfmt, "T", &IRECT{ mRECT.R - 20,mRECT.B - 10,mRECT.R - 10,mRECT.B });
    }

    vector<string> paramNames = m_unit->getParameterNames();
    char strbuf[256];
    sprintf(strbuf, "%s", m_unit->getName().c_str());
    pGraphics->DrawIText(&centertextfmt, strbuf, &IRECT{ m_x,m_y,m_x + m_size,m_y + 10 });
    for (int i = 0; i < paramNames.size(); i++)
    {
      pGraphics->FillCircle(&IColor{ 150,0,255,0 }, m_ports[i].add_rect.MW(), m_ports[i].add_rect.MH(), m_ports[i].add_rect.W() / 2, 0, true);
      pGraphics->DrawIText(&centertextfmt, "+", &m_ports[i].add_rect);
      pGraphics->FillCircle(&IColor{ 150,255,0,0 }, m_ports[i].scale_rect.MW(), m_ports[i].scale_rect.MH(), m_ports[i].scale_rect.W() / 2, 0, true);
      pGraphics->DrawIText(&centertextfmt, "x", &m_ports[i].scale_rect);
      sprintf(strbuf, "%s", paramNames[i].c_str());
      if (!m_unit->getParam(i).isHidden())
      {
        m_portLabels[i].Draw(pGraphics);
        pGraphics->DrawIText(&textfmtwhite, strbuf, &m_portLabels[i].getRect());
      }
      else
      {
        pGraphics->DrawIText(&textfmt, strbuf, &m_portLabels[i].getRect());
      }
    }
    return true;
  }

  NDPoint<2, int> UnitControl::getPos() const
  {
    return NDPoint<2, int>(m_x, m_y);
  }

  NDPoint<2, int> UnitControl::getPortPos(SelectedPort& port)
  {
    int portid = port.paramid;
    if (port.modaction == ADD)
    {
      return NDPoint<2, int>(m_ports[portid].add_rect.L + m_ports[portid].add_rect.W() / 2, m_ports[portid].add_rect.T + m_ports[portid].add_rect.H() / 2);
    }
    else if (port.modaction == SCALE)
    {
      return NDPoint<2, int>(m_ports[portid].scale_rect.L + m_ports[portid].scale_rect.W() / 2, m_ports[portid].scale_rect.T + m_ports[portid].scale_rect.H() / 2);
    }
    else
    {
      return NDPoint<2, int>(0, 0);
    }
  }

  NDPoint<2, int> UnitControl::getOutputPos() const
  {
    return NDPoint<2, int>(m_x + m_size, m_y + m_size / 2);
  }

  Unit* UnitControl::getUnit() const
  {
    return m_unit;
  }

  SelectedPort UnitControl::getSelectedPort(int x, int y)
  {
    SelectedPort selectedPort = { -1,SET };
    for (int i = 0; i < m_ports.size(); i++)
    {
      if (m_ports[i].add_rect.Contains(x, y))
      {
        selectedPort.paramid = i;
        selectedPort.modaction = ADD;
      }
      else if (m_ports[i].scale_rect.Contains(x, y))
      {
        selectedPort.paramid = i;
        selectedPort.modaction = SCALE;
      }
    }
    return selectedPort;
  }

  int UnitControl::getSelectedParam(int x, int y)
  {
    int selectedParam = -1;
    for (int i = 0; i < m_portLabels.size(); i++)
    {
      if (m_unit->getParam(i).isHidden())
      {
        continue;
      }
      if (m_portLabels[i].getRect().Contains(x, y))
      {
        selectedParam = i;
      }
    }
    return selectedParam;
  }

  void CircuitPanel::updateInstrument() const
  {
    m_vm->setMaxVoices(m_vm->getMaxVoices(), m_vm->getProtoInstrument());
  }

  void CircuitPanel::deleteUnit(int unitctrlid)
  {
    IPlugBase::IMutexLock lock(mPlug);
    WDL_MutexLock guilock(&mPlug->GetGUI()->mMutex);
    // Delete unit from instrument
    Unit* unit = m_unitControls[unitctrlid]->getUnit();
    int unitid = unit->getParent().getUnitId(unit);
    VOSIMSynth* vs = (VOSIMSynth*)mPlug;
    // reset oscilloscope input and trigger if necessary
    if (unitid == vs->m_oscilloscope_input_unit) {
      vs->m_oscilloscope_input_unit = -1;
    }
    if (unitid == vs->m_oscilloscope_trigger_unit) {
      vs->m_oscilloscope_trigger_unit = -1;
    }
    Instrument* instr = m_vm->getProtoInstrument();
    instr->removeUnit(instr->getUnitId(unit));
    // Delete unit controller
    UnitControl* unitctrl = m_unitControls[unitctrlid];
    m_unitControls.erase(unitctrlid);
    delete unitctrl;
  }

  void CircuitPanel::setSink(int unitctrlid)
  {
    Instrument* instr = m_vm->getProtoInstrument();
    Unit* unit = m_unitControls[unitctrlid]->getUnit();
    instr->setSinkId(instr->getUnitId(unit));
    m_unitControls[unitctrlid]->m_is_sink = true;
  }

  void CircuitPanel::OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    if (pMod->L) m_isMouseDown = 1;
    else if (pMod->R) m_isMouseDown = 2;
    m_lastMousePos = NDPoint<2, int>(x, y);
    m_lastClickPos = m_lastMousePos;
    m_lastSelectedUnit = getSelectedUnit(x, y);
    if (m_lastSelectedUnit >= 0)
    {
      UnitControl* unitCtrl = m_unitControls[m_lastSelectedUnit];
      m_lastSelectedPort = unitCtrl->getSelectedPort(x, y);
      m_lastSelectedParam = unitCtrl->getSelectedParam(x, y);

      if (pMod->L && m_lastSelectedParam >= 0)
      {
        m_currAction = MOD_PARAM;
      }
      else if (pMod->L && m_lastSelectedPort.paramid >= 0)
      {
        m_currAction = CONNECT;
      }
      else if (pMod->C && pMod->L)
      {
        m_currAction = RESIZE;
      }
      else if (pMod->L)
      {
        m_currAction = MOVE;
      }
    }
  }

  void CircuitPanel::createSourceUnit(int factoryid, int x, int y) {
    IPlugBase::IMutexLock lock(mPlug);
    WDL_MutexLock guilock(&mPlug->GetGUI()->mMutex);
    Instrument* instr = m_vm->getProtoInstrument();
    SourceUnit* srcunit = m_unitFactory->createSourceUnit(factoryid);
    int uid = instr->addSource(srcunit);
    m_unitControls[uid] = new UnitControl(mPlug, m_vm, srcunit, x, y);
    updateInstrument();
  }

  void CircuitPanel::createUnit(int factoryid, int x, int y) {
    IPlugBase::IMutexLock lock(mPlug);
    WDL_MutexLock guilock(&mPlug->GetGUI()->mMutex);
    Instrument* instr = m_vm->getProtoInstrument();
    Unit* unit = m_unitFactory->createUnit(factoryid);
    int uid = instr->addUnit(unit);
    m_unitControls[uid] = new UnitControl(mPlug, m_vm, unit, x, y);
    updateInstrument();
  }

  void CircuitPanel::OnMouseUp(int x, int y, IMouseMod* pMod)
  {
    WDL_MutexLock lock(&mPlug->GetGUI()->mMutex);
    Instrument* instr = m_vm->getProtoInstrument();
    int currSelectedUnit = getSelectedUnit(x, y);
    if (m_isMouseDown == 2 && currSelectedUnit == -1)
    { // Right clicking on open space
      IPopupMenu* selectedMenu = mPlug->GetGUI()->CreateIPopupMenu(&m_main_menu, x, y);
      if (selectedMenu == &m_sourceunit_menu)
      { // Create a source unit
        int itemChosen = selectedMenu->GetChosenItemIdx();
        createSourceUnit(itemChosen, x, y);
      }
      else if (selectedMenu == &m_unit_menu)
      { // Create a non-source unit
        int itemChosen = selectedMenu->GetChosenItemIdx();
        createUnit(itemChosen, x, y);
      }
    }
    else if (m_isMouseDown == 2 && currSelectedUnit >= 0)
    { // Right clicking on a unit
      IPopupMenu unitmenu;
      unitmenu.AddItem("Set sink");
      unitmenu.AddItem("Delete");
      unitmenu.AddItem("Set oscilloscope source");
      if (instr->isSourceUnit(currSelectedUnit))
      {
        unitmenu.AddSeparator();
        unitmenu.AddItem("Set oscilloscope trigger");
        unitmenu.AddItem("Set primary source");
      }
      Unit* unit = m_unitControls[currSelectedUnit]->getUnit();
      IPopupMenu* selectedmenu = mPlug->GetGUI()->CreateIPopupMenu(&unitmenu, x, y);
      if (selectedmenu == &unitmenu)
      {
        IPlugBase::IMutexLock lock(mPlug);
        int selectedItem = selectedmenu->GetChosenItemIdx();
        if (selectedItem == 0)
        { // Set sink
          setSink(currSelectedUnit);
          updateInstrument();
        }
        else if (selectedItem == 1)
        { // Delete unit
          deleteUnit(currSelectedUnit);
          updateInstrument();
        }
        else if (selectedItem == 2)
        { // Set oscilloscope source
          VOSIMSynth* vs = (VOSIMSynth*)mPlug;
          vs->m_oscilloscope_input_unit = instr->getUnitId(unit);
        }
        else if (selectedItem == 4)
        { // Set oscilloscope trigger
          VOSIMSynth* vs = (VOSIMSynth*)mPlug;
          vs->m_oscilloscope_trigger_unit = instr->getUnitId(unit);
        }
        else if (selectedItem == 5)
        { // Set primary source
          instr->resetPrimarySource(instr->getUnitId(unit));
          updateInstrument();
        }
      }
    }
    else if (m_currAction == CONNECT && currSelectedUnit >= 0)
    {
      IPlugBase::IMutexLock lock(mPlug);
      bool result = m_vm->getProtoInstrument()->addConnection({ currSelectedUnit, m_lastSelectedUnit, m_lastSelectedPort.paramid, m_lastSelectedPort.modaction });
      updateInstrument();
    }
    m_currAction = NONE;
    m_isMouseDown = 0;
  }

  void CircuitPanel::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
  {
    NDPoint<2, int> currMousePos = NDPoint<2, int>(x, y);
    if (m_lastSelectedUnit >= 0)
    {
      UnitControl* unitCtrl = m_unitControls[m_lastSelectedUnit];
      NDPoint<2, int> unitPos = unitCtrl->getPos();
      if (m_currAction == MOD_PARAM)
      {
        unitCtrl->m_portLabels[m_lastSelectedParam].OnMouseDrag(x,y,dX,dY,pMod);
      }
      else if (m_currAction == RESIZE)
      {
        int newsize = NDPoint<2, int>(x, y).distFrom(unitPos);
        unitCtrl->resize(newsize);
      }
      else if (m_currAction == MOVE)
      {
        NDPoint<2, int> newUnitPos = currMousePos - m_lastMousePos + unitPos;
        unitCtrl->move(newUnitPos[0], newUnitPos[1]);
      }
    }
    m_lastMousePos = currMousePos;
  }

  void CircuitPanel::OnMouseDblClick(int x, int y, IMouseMod* pMod)
  {}

  bool CircuitPanel::Draw(IGraphics* pGraphics)
  {
    WDL_MutexLock lock(&pGraphics->mMutex);
    pGraphics->FillIRect(&(IColor)palette[0], &mRECT);
    for (std::pair<int, UnitControl*> unitpair : m_unitControls)
    {
      unitpair.second->Draw(pGraphics);
    }
    Instrument* instr = m_vm->getProtoInstrument();
    for (std::pair<int, UnitControl*> unitpair : m_unitControls)
    {
      const vector<ConnectionMetadata>& connections = instr->getConnectionsTo(unitpair.first);
      for (int j = 0; j < connections.size(); j++)
      {
        NDPoint<2, int> pt1 = m_unitControls[connections[j].srcid]->getOutputPos();
        NDPoint<2, int> pt2 = m_unitControls[connections[j].targetid]->getPortPos(SelectedPort{ connections[j].portid,connections[j].action });
        pGraphics->DrawLine(&IColor(255, 255, 255, 255), pt1[0], pt1[1], pt2[0], pt2[1], 0, true);
      }
    }
    if (m_currAction == CONNECT)
    {
      pGraphics->DrawLine(&IColor(255, 255, 255, 255), m_lastClickPos[0], m_lastClickPos[1], m_lastMousePos[0], m_lastMousePos[1], 0, true);
    }
    return true;
  }

  int CircuitPanel::getSelectedUnit(int x, int y)
  {
    int selectedUnit = -1;
    for (std::pair<int, UnitControl*> unitpair : m_unitControls)
    {
      if (unitpair.second->IsHit(x, y))
      {
        selectedUnit = unitpair.first;
      }
    }
    return selectedUnit;
  }

  ByteChunk CircuitPanel::serialize() const
  {
    ByteChunk serialized;

    unsigned int numunits = m_unitControls.size();
    serialized.PutBytes(&numunits, sizeof(unsigned int));
    for (std::pair<int, UnitControl*> ctrlpair : m_unitControls)
    {
      serialized.PutChunk(&serializeUnitControl(ctrlpair.first));
    }

    Instrument* instr = m_vm->getProtoInstrument();
    for (std::pair<int, UnitControl*> ctrlpair : m_unitControls)
    {
      int unitid = instr->getUnitId(ctrlpair.second->m_unit);
      const vector<ConnectionMetadata> connections = instr->getConnectionsTo(unitid);

      unsigned int numconnections = connections.size();
      serialized.PutBytes(&numconnections, sizeof(unsigned int));
      for (int j = 0; j < connections.size(); j++)
      {
        serialized.Put<ConnectionMetadata>(&connections[j]);
      }
    }
    return serialized;
  }

  int CircuitPanel::unserialize(ByteChunk* serialized, int startPos)
  {
    int chunkpos = startPos;
    Instrument* instr = m_vm->getProtoInstrument();
    vector<int> unitctrlids;
    for (std::pair<int, UnitControl*> ctrlpair : m_unitControls) {
      unitctrlids.push_back(ctrlpair.first);
    }
    for (int i = 0; i < unitctrlids.size(); i++) {
      deleteUnit(unitctrlids[i]);
    }

    // Unserialize units
    unsigned int numunits;
    chunkpos = serialized->Get<unsigned int>(&numunits, chunkpos);
    for (int i = 0; i < numunits; i++) {
      chunkpos = unserializeUnitControl(serialized, chunkpos);
    }

    // Unserialize connections
    for (int i = 0; i < numunits; i++) {
      unsigned int numConns;
      chunkpos = serialized->Get<unsigned int>(&numConns, chunkpos);
      for (int j = 0; j < numConns; j++) {
        ConnectionMetadata conn;
        chunkpos = serialized->Get<ConnectionMetadata>(&conn, chunkpos);
        instr->addConnection(conn);
      }
    }
    updateInstrument();
    return chunkpos;
  }
  ByteChunk CircuitPanel::serializeUnitControl(int ctrlidx) const
  {
    ByteChunk serialized;
    UnitControl* uctrl = m_unitControls.at(ctrlidx);
    Instrument* instr = m_vm->getProtoInstrument();
    unsigned int unitClassId = uctrl->m_unit->getClassIdentifier();
    int unitid = instr->getUnitId(uctrl->m_unit);
    bool isSource = instr->isSourceUnit(unitid);
    bool isPrimarySource = isSource ? instr->isPrimarySource(unitid) : false;

    serialized.Put<unsigned int>(&unitClassId);
    serialized.Put<int>(&unitid);
    serialized.Put<bool>(&isSource);
    serialized.Put<bool>(&isPrimarySource);
    serialized.Put<bool>(&uctrl->m_is_sink);

    serialized.Put<unsigned int>(&uctrl->m_nParams);
    for (int i = 0; i < uctrl->m_nParams; i++) {
      double paramval = uctrl->m_unit->getParam(i);
      serialized.Put<double>(&paramval);
    }

    serialized.Put<int>(&uctrl->m_size);
    serialized.Put<int>(&uctrl->m_x);
    serialized.Put<int>(&uctrl->m_y);
    return serialized;
  }

  int CircuitPanel::unserializeUnitControl(ByteChunk* chunk, int startPos)
  {
    IPlugBase::IMutexLock lock(mPlug);
    WDL_MutexLock guilock(&mPlug->GetGUI()->mMutex);
    int chunkpos = startPos;
    unsigned int unitClassId;
    int unitid;
    bool isSource, isPrimarySource, isSink;
    unsigned int numparams;
    int x, y;
    int size;
    Unit* unit;
    chunkpos = chunk->Get<unsigned int>(&unitClassId, chunkpos);
    chunkpos = chunk->Get<int>(&unitid, chunkpos);
    chunkpos = chunk->Get<bool>(&isSource, chunkpos);
    chunkpos = chunk->Get<bool>(&isPrimarySource, chunkpos);
    chunkpos = chunk->Get<bool>(&isSink, chunkpos);
    unit = isSource ? m_unitFactory->createSourceUnit(unitClassId) : m_unitFactory->createUnit(unitClassId);

    chunkpos = chunk->Get<unsigned int>(&numparams, chunkpos);
    for (int i = 0; i < numparams; i++) {
      double paramval;
      chunkpos = chunk->Get<double>(&paramval, chunkpos);
      unit->modifyParameter(i, paramval, SET);
    }
    chunkpos = chunk->Get<int>(&size, chunkpos);
    chunkpos = chunk->Get<int>(&x, chunkpos);
    chunkpos = chunk->Get<int>(&y, chunkpos);

    Instrument* instr = m_vm->getProtoInstrument();
    int uid = isSource ? instr->addSource(dynamic_cast<SourceUnit*>(unit), unitid) : instr->addUnit(unit, unitid);
    m_unitControls[uid] = new UnitControl(mPlug, m_vm, unit, x, y, size);
    if (isSink) setSink(uid);
    if (isPrimarySource) instr->setPrimarySource(uid);
    updateInstrument();
    return chunkpos;
  }
}