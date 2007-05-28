#ifndef COUNTERMANAGER_H_
#define COUNTERMANAGER_H_

#include "icounter.h"
#include "iradiant.h"
#include <boost/shared_ptr.hpp>

namespace map {

class Counter : 
	public ICounter
{
	Observer* _observer;
	std::size_t _count;
public:
	Counter(Observer* observer = NULL) :
		_observer(observer), 
		_count(0)
	{}
	
	void increment() {
		++_count;
		
		if (_observer != NULL) {
			_observer->countChanged();
		}
	}
	
	void decrement() {
		--_count;
		
		if (_observer != NULL) {
			_observer->countChanged();
		}
	}
	
	std::size_t get() const {
		return _count;
	}
};

class CounterManager :
	public ICounter::Observer
{
	typedef boost::shared_ptr<Counter> CounterPtr;
	typedef std::map<CounterType, CounterPtr> CounterMap;
	CounterMap _counters;
public:
	CounterManager() {
		// Create the counter object
		_counters[counterBrushes] = CounterPtr(new Counter(this));
		_counters[counterPatches] = CounterPtr(new Counter(this));
		_counters[counterEntities] = CounterPtr(new Counter(this));		
	}
	
	ICounter& get(CounterType counter) {
		if (_counters.find(counter) == _counters.end()) {
			throw std::runtime_error("Counter ID not found.");
		}
		return *_counters[counter];
	}
	
	// ICounter::Observer implementation
	void countChanged() {
		int brushCount(_counters[counterBrushes]->get());
		int patchCount(_counters[counterPatches]->get());
		int entityCount(_counters[counterEntities]->get());
		
		std::string text = "Brushes: " + intToStr(brushCount);
		text += " Patches: " + intToStr(patchCount);
		text += " Entities: " + intToStr(entityCount);
		
		g_pParentWnd->SetStatusText(g_pParentWnd->m_brushcount_status, text);
	}
};

} // namespace map

#endif /*COUNTERMANAGER_H_*/
