#pragma once

#include "routing_common/vehicle_model.hpp"

namespace routing
{

class PedestrianModel : public VehicleModel
{
public:
  PedestrianModel();
  explicit PedestrianModel(VehicleModel::LimitsInitList const & speedLimits);

  /// VehicleModelInterface overrides:
  SpeedKMpH GetSpeed(FeatureType & f, SpeedParams const & speedParams) const override;
  bool IsOneWay(FeatureType &) const override { return false; }
  SpeedKMpH const & GetOffroadSpeed() const override;

  static PedestrianModel const & AllLimitsInstance();
};

class PedestrianModelFactory : public VehicleModelFactory
{
public:
  // TODO: remove countryParentNameGetterFn default value after removing unused pedestrian routing
  // from road_graph_router
  PedestrianModelFactory(CountryParentNameGetterFn const & countryParentNameGetterFn = {});
};
}  // namespace routing
