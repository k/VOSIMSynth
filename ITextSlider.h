#pragma once
#include "IPlug_include_in_plug_hdr.h"
#include "IGraphics.h"
#include "UnitParameter.h"
#include "VoiceManager.h"

namespace syn
{
  class ITextSlider : public IControl
  {
  public:

    ITextSlider(IPlugBase* pPlug, VoiceManager* vm, IRECT pR, int unitid, int paramid) :
      m_unitid(unitid),
      m_paramid(paramid),
      m_vm(vm),
      m_rect(pR),
      IControl(pPlug,pR)
    {
      UnitParameter& param = m_vm->getProtoInstrument()->getUnit(unitid).getParam(paramid);
      m_min = param.getMin();
      m_max = param.getMax();
      m_value = (param - m_min) / (m_max - m_min);
      m_name = param.getName();
    }

    virtual ~ITextSlider()
    {}

    void setRect(IRECT& newrect)
    {
      m_rect = newrect;
    }

    IRECT& getRect()
    {
      return m_rect;
    }

    virtual void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod) override
    {
      double scale = pMod->S ? 1./10 : 1.0;
      m_value += dX/(double)m_rect.W()*scale;
      if (m_value > 1) m_value = 1;
      else if (m_value < 0) m_value = 0;
      m_vm->modifyParameter(m_unitid, m_paramid, m_value*(m_max - m_min) + m_min, SET);
    }

    bool Draw(IGraphics* pGraphics)
    {
      IText textfmt{ 12,&IColor{255,255,255,255},"Helvetica",IText::kStyleNormal,IText::kAlignNear,0,IText::kQualityClearType };
      IText textfmtfar{ 12,&IColor{ 255,255,255,255 },"Helvetica",IText::kStyleNormal,IText::kAlignFar,0,IText::kQualityClearType };
      pGraphics->FillIRect(&IColor{ 255,100,50,125 }, &m_rect);
      pGraphics->FillIRect(&IColor{ 255,200,150,225 }, &IRECT{ m_rect.L,m_rect.T,m_rect.L+(int)(m_rect.W()*m_value),m_rect.B });

      UnitParameter& param = m_vm->getProtoInstrument()->getUnit(m_unitid).getParam(m_paramid);

      string paramstr = param.getString();
      char strbuf[256];
      sprintf(strbuf,"%s",paramstr.c_str());
      pGraphics->DrawIText(&textfmtfar, strbuf, &IRECT{ m_rect.L,m_rect.T,m_rect.L + m_rect.W(),m_rect.B });

      return true;
    }

  protected:
    string m_name;
    int m_unitid;
    int m_paramid;
    double m_value;
    double m_min, m_max;
    VoiceManager* m_vm;
    IRECT m_rect;
  };
}