// $Id: perf_log.h,v 1.6 2008/06/08 03:54:29 gdiso Exp $

// The libMesh Finite Element Library.
// Copyright (C) 2002-2007  Benjamin S. Kirk, John W. Peterson
  
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
  
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA



#ifndef __perflog_h__
#define __perflog_h__

// enable log
//#define ENABLE_PERFORMANCE_LOGGING


#ifdef ENABLE_PERFORMANCE_LOGGING

// Local includes
#include "genius_common.h"
#include "o_string_stream.h"

// C++ includes
#include <string>
#include <stack>
#include <map>
#include <sys/time.h>

#ifdef HAVE_LOCALE
#include <locale>
#endif

// Forward Declarations
// class OStringStream;


/**
 * The \p PerfData class simply contains the performance
 * data that is recorded for individual events.
 */

// ------------------------------------------------------------
// PerfData class definition
class PerfData
{
 public:

  /**
   * Constructor.  Initializes data to be empty.
   */
  PerfData () :
    tot_time(0.),
    count(0),
    open(false),
    called_recursively(0)
    {}

  
  /**
   * Total time spent in this event.
   */
  double tot_time;

  /**
   * Structure defining when the event
   * was last started.
   */
  struct timeval tstart;

  /**
   * The number of times this event has
   * been executed
   */
  unsigned int count;

  /**
   * Flag indicating if we are currently
   * monitoring this event.  Should only
   * be true while the event is executing.
   */
  bool open;

  void   start ();
  void   restart ();
  double pause ();
  double stopit ();
    
  int called_recursively;
    
};




/**
 * The \p PerfLog class allows monitoring of specific events.
 * An event is defined by a unique string that functions as
 * a label.  Each time the event is executed data are recorded.
 * This class is particulary useful for finding performance
 * bottlenecks. 
 *
 */

// ------------------------------------------------------------
// PerfLog class definition
class PerfLog
{

 public:

  /**
   * Constructor.  \p label_name is the name of the object, which
   * will bw print in the log to distinguish it from other objects.
   * \p log_events is a flag to optionally
   * disable logging.  You can use this flag to turn off
   * logging without touching any other code.
   */
  PerfLog(const std::string& label_name="",
	  const bool log_events=true);

  /**
   * Destructor. Calls \p clear() and \p print_log().
   */
  ~PerfLog();
  
  /**
   * Clears all the internal data and returns the
   * data structures to a pristine state.  This function
   * checks to see if it is currently monitoring any
   * events, and if so errors.  Be sure you are not
   * logging any events when you call this function.
   */
  void clear();

  /**
   * Disables performance logging for an active object.
   */
  void disable_logging() { log_events = false; }

  /**
   * Enables performance logging for an active object.
   */
  void enable_logging() { log_events = true; }


  /**
   * Push the event \p label onto the stack, pausing any active event.
   */
  void push (const std::string &label,
	     const std::string &header="");
  
  /**
   * Pop the event \p label off the stack, resuming any lower event.
   */
  void pop (const std::string &label,
	    const std::string &header="");
  
  /**
   * Start monitoring the event named \p label.
   */
  void start_event(const std::string &label,
		   const std::string &header="");

  /**
   * Stop monitoring the event named \p label.
   */
  void stop_event(const std::string &label,
		  const std::string &header="");

  /**
   * Suspend monitoring of the event. 
   */
  void pause_event(const std::string &label,
		   const std::string &header="");

  /**
   * Restart monitoring the event.
   */
  void restart_event(const std::string &label,
		     const std::string &header="");
  
  /**
   * @returns a string containing:
   * (1) Basic machine information (if first call)
   * (2) The performance log
   */
  std::string get_log() const;
  
  /**
   * @returns a string containing ONLY the information header.
   */
  std::string get_info_header() const;

  /**
   * @returns a string containing ONLY the log information
   */
  std::string get_perf_info() const;
  
  /**
   * Print the log.
   */
  void print_log() const;

  /**
   * @returns the total time spent on this event.
   */
  double get_total_time() const
    {return total_time;}
 
   
 private:

  
  /**
   * The label for this object.
   */
  const std::string label_name;

  /**
   * Flag to optionally disable all logging.
   */
  bool log_events;

  /**
   * The total running time for recorded events.
   */  
  double total_time;
  
  /**
   * The time we were constructed or last cleared.
   */
  struct timeval tstart;
  
  /**
   * The actual log.
   */
  std::map<std::pair<std::string,
		     std::string>,
	   PerfData> log;

  /**
   * A stack to hold the current performance log trace.
   */
  std::stack<PerfData*> log_stack;
  
  /**
   * Flag indicating if print_log() has been called.
   * This is used to print a header with machine-specific
   * data the first time that print_log() is called.
   */
  static bool called;
  
  /**
   * Prints a line of 'n' repeated characters 'c'
   * to the output string stream "out".
   */
  void _character_line(const unsigned int n,
		       const char c,
		       OStringStream& out) const;
};



// ------------------------------------------------------------
// PerfData class member funcions
inline
void PerfData::start ()
{
  this->count++;
  this->called_recursively++;
  gettimeofday (&(this->tstart), NULL);
}



inline
void PerfData::restart ()
{
  gettimeofday (&(this->tstart), NULL);
}



inline
double PerfData::pause ()
{
  // save the start times, reuse the structure we have rather than create
  // a new one.
  const double
    tstart_tv_sec  = this->tstart.tv_sec,
    tstart_tv_usec = this->tstart.tv_usec;
  
  gettimeofday (&(this->tstart), NULL);
  
  const double elapsed_time = (static_cast<double>(this->tstart.tv_sec  - tstart_tv_sec) +
			       static_cast<double>(this->tstart.tv_usec - tstart_tv_usec)*1.e-6);      
  
  this->tot_time += elapsed_time;

  return elapsed_time;
}


inline
double PerfData::stopit ()
{
  // stopit is just like pause except decriments the 
  // recursive call counter
  
  this->called_recursively--;
  return this->pause();
}



// ------------------------------------------------------------
// PerfLog class inline member funcions
inline
void PerfLog::push (const std::string &label,
		    const std::string &header)
{
  if (this->log_events)
    {
      // Get a reference to the event data to avoid
      // repeated map lookups
      PerfData *perf_data = &(log[std::make_pair(header,label)]);

      if (!log_stack.empty())
	total_time += 
	  log_stack.top()->pause();
      
      perf_data->start();
      log_stack.push(perf_data);
    }
}



inline
void PerfLog::pop (const std::string &label,
		   const std::string &header)
{
  if (this->log_events)
    {
      assert (!log_stack.empty());

#ifndef NDEBUG
      PerfData *perf_data = &(log[std::make_pair(header,label)]);
      assert (perf_data == log_stack.top());
#endif

      total_time += log_stack.top()->stopit();

      log_stack.pop();

      if (!log_stack.empty())
	log_stack.top()->restart();
    }
}

// Typedefs we might need
#ifdef HAVE_LOCALE
typedef std::ostreambuf_iterator<char, std::char_traits<char> > TimeIter;
typedef std::time_put<char, TimeIter> TimePut;
#endif

#endif //#ifdef ENABLE_PERFORMANCE_LOGGING



#ifdef ENABLE_PERFORMANCE_LOGGING
extern PerfLog  perflog;
#  define START_LOG(a,b)   { perflog.push(a,b); }
#  define STOP_LOG(a,b)    { perflog.pop(a,b); }
#  define PAUSE_LOG(a,b)   { deprecated(); }
#  define RESTART_LOG(a,b) { deprecated{}; }

#else

#  define START_LOG(a,b)   {}
#  define STOP_LOG(a,b)    {}
#  define PAUSE_LOG(a,b)   {}
#  define RESTART_LOG(a,b) {}

#endif // #ifdef ENABLE_PERFORMANCE_LOGGING



#endif // #ifndef __perflog_h__
