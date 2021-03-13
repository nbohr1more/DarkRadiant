#pragma once

#include "ishaders.h"
#include <wx/checkbox.h>

namespace ui
{

template<typename Source>
class Binding
{
private:
    Source _source;

public:
    using Ptr = std::shared_ptr<Binding>;

    virtual ~Binding()
    {}

    const Source& getSource() const
    {
        return _source;
    }

    void setSource(const Source& source)
    {
        _source = source;
        onSourceChanged();
    }

    virtual void updateFromSource() = 0;

protected:
    virtual void onSourceChanged()
    {}
};

template<typename Source, typename Target>
class TwoWayBinding :
    public Binding<Source>
{
private:
    std::function<Target()> _acquireTarget;

protected:
    bool _blockUpdates;

protected:
    TwoWayBinding(const std::function<Target()>& acquireTarget) :
        _acquireTarget(acquireTarget),
        _blockUpdates(false)
    {}

    Target getTarget()
    {
        if (_blockUpdates)
        {
            return Target();
        }

        return _acquireTarget();
    }
};

template<typename Source>
class CheckBoxBinding :
    public Binding<Source>
{
private:
    wxCheckBox* _checkbox;
    std::function<bool(const Source&)> _loadFunc;
    std::function<void(const Source&, bool)> _saveFunc;

public:
    CheckBoxBinding(wxCheckBox* checkbox, 
        const std::function<bool(const Source&)>& loadFunc) :
        CheckBoxBinding(checkbox, loadFunc, std::function<void(const Source&, bool)>())
    {}

    CheckBoxBinding(wxCheckBox* checkbox,
        const std::function<bool(const Source&)>& loadFunc,
        const std::function<void(const Source&, bool)>& saveFunc) :
        _checkbox(checkbox),
        _loadFunc(loadFunc),
        _saveFunc(saveFunc)
    {
        if (_saveFunc)
        {
            _checkbox->Bind(wxEVT_CHECKBOX, &CheckBoxBinding::onCheckedChanged, this);
        }
    }

    virtual ~CheckBoxBinding()
    {
        if (_saveFunc)
        {
            _checkbox->Unbind(wxEVT_CHECKBOX, &CheckBoxBinding::onCheckedChanged, this);
        }
    }

    virtual void updateFromSource() override
    {
        if (!Binding<Source>::getSource())
        {
            _checkbox->SetValue(false);
            return;
        }

        _checkbox->SetValue(_loadFunc(Binding<Source>::getSource()));
    }

private:
    void onCheckedChanged(wxCommandEvent& ev)
    {
        _saveFunc(Binding<Source>::getSource(), _checkbox->IsChecked());
    }
};

}
