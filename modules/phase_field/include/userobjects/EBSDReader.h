/****************************************************************/
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*          All contents are licensed under LGPL V2.1           */
/*             See LICENSE for full restrictions                */
/****************************************************************/
#ifndef EBSDREADER_H
#define EBSDREADER_H

#include "EulerAngleProvider.h"
#include "EBSDAccessFunctors.h"

class EBSDReader;

template<>
InputParameters validParams<EBSDReader>();

/**
 * A GeneralUserObject that reads an EBSD file and stores the centroid
 * data in a data structure which indexes on element centroids.
 *
 * Grains are indexed through multiple schemes:
 *  * feature_id   The grain number in the EBSD data file
 *  * global_id    The index into the global average data (this is feature_id shifted back by feature_id_origin)
 *  * local_id     The index into the per-phase grain list. This is only unique when combined with phase number
 *
 * Phases are referred to using the numbers in the EBSD data file. In case the phase number in the data file
 * starts at 1 the phase 0 will simply contain no grains.
 */
class EBSDReader : public EulerAngleProvider, public EBSDAccessFunctors
{
public:
  EBSDReader(const InputParameters & params);
  virtual ~EBSDReader();

  virtual void readFile();

  /**
   * Called before execute() is ever called so that data can be cleared.
   */
  virtual void initialize() {}

  /**
   * Called when this object needs to compute something.
   */
  virtual void execute() {}

  /**
   * Called _after_ execute(), could be used to do MPI communictaion.
   */
  virtual void finalize() {}

  /**
   * Get the requested type of data at the point p.
   */
  const EBSDPointData & getData(const Point & p) const;

  /**
   * Get the requested type of average data for feature number i.
   */
  const EBSDAvgData &  getAvgData(unsigned int i) const;

  /**
   * Get the requested type of average data for a given phase and grain.
   */
  const EBSDAvgData &  getAvgData(unsigned int phase, unsigned int local_id) const;

  /**
   * EulerAngleProvider interface implementation to fetch a triplet of Euler angles
   */
  virtual const EulerAngles & getEulerAngles(unsigned int) const;

  /**
   * Return the total number of grains
   */
  virtual unsigned int getGrainNum() const;

  /**
   * Return the total number of phases
   */
  virtual unsigned int getPhaseNum() const { return _global_id.size(); }

  /**
   * Return the number of grains in a given phase
   */
  unsigned int getGrainNum(unsigned int phase) const;

  /**
   * Return the feature id (global grain number) for a given phase and phase grain number
   */
  unsigned int getFeatureID(unsigned int phase, unsigned int local_id) const { return _avg_data[_global_id[phase][local_id]].grain; }

  /**
   * Return the feature id (global grain number) for a given phase and phase grain number
   */
  unsigned int getGlobalID(unsigned int phase, unsigned int local_id) const { return _global_id[phase][local_id]; }

  /// Factory function to return a point functor specified by name
  MooseSharedPointer<EBSDPointDataFunctor> getPointDataAccessFunctor(const MooseEnum & field_name) const;
  /// Factory function to return a average functor specified by name
  MooseSharedPointer<EBSDAvgDataFunctor> getAvgDataAccessFunctor(const MooseEnum & field_name) const;

  /**
   * Returns a map consisting of the node index followd by
   * a vector of all grain weights for that node. Needed by ReconVarIC
   */
  const std::map<dof_id_type, std::vector<Real> > & getNodeToGrainWeightMap() const;

  /**
   * Returns a map consisting of the node index followd by
   * a vector of all phase weights for that node. Needed by ReconPhaseVarIC
   */
  const std::map<dof_id_type, std::vector<Real> > & getNodeToPhaseWeightMap() const;

protected:
  // MooseMesh Variables
  MooseMesh & _mesh;
  NonlinearSystem & _nl;

  /// Variables needed to determine reduced order parameter values
  unsigned int _feature_num;
  Point _bottom_left;
  Point _top_right;
  Point _range;

  /// number of additional custom data columns
  unsigned int _custom_columns;

  /// Logically three-dimensional data indexed by geometric points in a 1D vector
  std::vector<EBSDPointData> _data;

  /// Averages by feature ID
  std::vector<EBSDAvgData> _avg_data;

  /// Euler Angles by feature ID
  std::vector<EulerAngles> _avg_angles;

  /// map from feature_id to global_id
  std::map<unsigned int, unsigned int> _global_id_map;

  /// global ID for given phases and grains
  std::vector<std::vector<unsigned int> > _global_id;

  /// Map of grain weights per node
  std::map<dof_id_type, std::vector<Real> > _node_to_grain_weight_map;

  /// Map of phase weights per node
  std::map<dof_id_type, std::vector<Real> > _node_to_phase_weight_map;

  /// Dimension of the problem domain
  unsigned int _mesh_dimension;

  /// The number of values in the x, y and z directions.
  unsigned _nx, _ny, _nz;

  /// The spacing of the values in x, y and z directions.
  Real _dx, _dy, _dz;

  /// Grid origin
  Real _minx, _miny, _minz;

  /// Maximum grid extent
  Real _maxx, _maxy, _maxz;

  /// Computes a global index in the _data array given an input *centroid* point
  unsigned indexFromPoint(const Point & p) const;

  /// Transfer the index into the _avg_data array from given index
  unsigned indexFromIndex(unsigned int var) const;

  /// Build grain and phase weight maps
  void buildNodeWeightMaps();
};

#endif // EBSDREADER_H
