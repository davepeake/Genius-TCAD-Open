/********************************************************************************/
/*     888888    888888888   88     888  88888   888      888    88888888       */
/*   8       8   8           8 8     8     8      8        8    8               */
/*  8            8           8  8    8     8      8        8    8               */
/*  8            888888888   8   8   8     8      8        8     8888888        */
/*  8      8888  8           8    8  8     8      8        8            8       */
/*   8       8   8           8     8 8     8      8        8            8       */
/*     888888    888888888  888     88   88888     88888888     88888888        */
/*                                                                              */
/*       A Three-Dimensional General Purpose Semiconductor Simulator.           */
/*                                                                              */
/*                                                                              */
/*  Copyright (C) 2007-2008                                                     */
/*  Cogenda Pte Ltd                                                             */
/*                                                                              */
/*  Please contact Cogenda Pte Ltd for license information                      */
/*                                                                              */
/*  Author: Gong Ding   gdiso@ustc.edu                                          */
/*                                                                              */
/********************************************************************************/



#ifndef __particle_source_h__
#define __particle_source_h__

#include "auto_ptr.h"
#include "parser.h"
#include "point.h"
#include "interpolation_base.h"

class SimulationSystem;

/**
 * set the carrier generation of Particle
 */
class Particle_Source
{
public:
  /**
   *  constructor, do nothing
   */
  Particle_Source(SimulationSystem &system) : _system(system) {}

  /**
   * destructor
   */
  virtual ~Particle_Source()  {}

  /**
   * calculate carrier generation at time t
   */
  virtual double carrier_generation(double t) const
  {
    if( t>= _t0 && (t-_t_max)*(t-_t_max)/(_t_char*_t_char)<30)
      return exp(-(t-_t_max)*(t-_t_max)/(_t_char*_t_char));
    return 0;
  }

  /**
   * assign PatG to mesh node
   */
  virtual void update_system()=0;

protected:

 /**
  * I should know something about simulation system
  */
 SimulationSystem & _system;

  /**
  * the particle incident time
  */
 double _t0;

 /**
  * the time electron-hole generation rate reaches its max value
  */
 double _t_max;

 /**
  * we assume electron-hole generation rate has a Gauss distribution
  * this is the characteristic time
  */
 double _t_char;

 /**
  * how much energy can generate electron-hole pair
  */
 double _quan_eff;
};


/**
 * set electron/hole generation by reading energy deposit from external file, old code
 */
class  Particle_Source_DataFile : public Particle_Source
{

public:
  /**
   *  constructor, do nothing
   */
  Particle_Source_DataFile(SimulationSystem &, const Parser::Card &);

  /**
   * destructor
   */
  ~Particle_Source_DataFile() {}

  /**
   * assign PatG to mesh node
   */
  virtual void update_system();

private:

 void set_particle_profile_fromfile2d(const Parser::Card &);

 void set_particle_profile_fromfile3d(const Parser::Card &);

 AutoPtr<InterpolationBase> interpolator;

};



/**
 * set electron/hole generation by single particle track, old code
 */
class  Particle_Source_Analytic : public Particle_Source
{
public:
  /**
   *  constructor, do nothing
   */
  Particle_Source_Analytic(SimulationSystem &, const Parser::Card &);

  /**
   * destructor
   */
  ~Particle_Source_Analytic() {}

  /**
   * assign PatG to mesh node
   */
  virtual void update_system();

private:

  /**
   * particle incident point
   */
  Point _start;

  /**
   * particle incident direction
   */
  Point _dir;

  /**
   * lateral char. length
   */
  double _lateral_char;

  /**
   * the length of particle trace
   */
  double _length;

  /**
   * linear energy transfer
   */
  double _LET;

};



/**
 * set electron/hole generation by geading particle track from external file, new code
 */
class  Particle_Source_Track : public Particle_Source
{
public:
  /**
   *  constructor, parse track file
   */
  Particle_Source_Track(SimulationSystem &, const Parser::Card &);

  /**
   * destructor
   */
  ~Particle_Source_Track() {}

  /**
   * assign PatG to mesh node
   */
  virtual void update_system();

private:

  /// track struct
  struct track_t
  {
    Point start;
    Point end;
    double energy;
  };

  std::vector<track_t> _tracks;

  void _read_particle_profile_track(const std::string &file);

  /**
   * lateral char. length
   */
  double _lateral_char;


};


#endif // #define __particle_source_h__
