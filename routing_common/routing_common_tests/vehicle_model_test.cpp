#include "testing/testing.hpp"

#include "routing_common/car_model_coefs.hpp"
#include "routing_common/maxspeed_conversion.hpp"
#include "routing_common/vehicle_model.hpp"
#include "routing_common/car_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include "platform/measurement_utils.hpp"

#include "base/math.hpp"

namespace vehicle_model_test
{
using namespace routing;
using namespace std;

HighwayBasedSpeeds const kDefaultSpeeds = {
    {HighwayType::HighwayTrunk, InOutCitySpeedKMpH(100.0 /* in city */, 150.0 /* out city */)},
    {HighwayType::HighwayPrimary, InOutCitySpeedKMpH(90.0 /* in city */, 120.0 /* out city */)},
    {HighwayType::HighwaySecondary,
     InOutCitySpeedKMpH(SpeedKMpH(80.0 /* weight */, 70.0 /* eta */) /* in and out city*/)},
    {HighwayType::HighwayResidential,
     InOutCitySpeedKMpH(SpeedKMpH(45.0 /* weight */, 55.0 /* eta */) /* in city */,
                        SpeedKMpH(50.0 /* weight */, 60.0 /* eta */) /* out city */)},
    {HighwayType::HighwayService,
     InOutCitySpeedKMpH(SpeedKMpH(47.0 /* weight */, 36.0 /* eta */) /* in city */,
                        SpeedKMpH(50.0 /* weight */, 40.0 /* eta */) /* out city */)}
};

HighwayBasedFactors const kDefaultFactors = {
    {HighwayType::HighwayTrunk, InOutCityFactor(1.0)},
    {HighwayType::HighwayPrimary, InOutCityFactor(1.0)},
    {HighwayType::HighwaySecondary, InOutCityFactor(1.0)},
    {HighwayType::HighwayResidential, InOutCityFactor(0.5)}
};

VehicleModel::LimitsInitList const kTestLimits = {
  {HighwayType::HighwayTrunk, true},
  {HighwayType::HighwayPrimary, true},
  {HighwayType::HighwaySecondary, true},
  {HighwayType::HighwayResidential, true},
  {HighwayType::HighwayService, false}
};

VehicleModel::SurfaceInitList const kCarSurface = {
    {{"psurface", "paved_good"}, {0.8 /* weightFactor */, 0.9 /* etaFactor */}},
    {{"psurface", "paved_bad"}, {0.4, 0.5}},
    {{"psurface", "unpaved_good"}, {0.6, 0.8}},
    {{"psurface", "unpaved_bad"}, {0.2, 0.2}}
};

class VehicleModelTest
{
public:
  VehicleModelTest()
  {
    classificator::Load();
    auto const & c = classif();

    primary = c.GetTypeByPath({"highway", "primary"});
    secondary = c.GetTypeByPath({"highway", "secondary"});
    secondaryBridge = c.GetTypeByPath({"highway", "secondary", "bridge"});
    secondaryTunnel = c.GetTypeByPath({"highway", "secondary", "tunnel"});
    residential = c.GetTypeByPath({"highway", "residential"});

    oneway = c.GetTypeByPath({"hwtag", "oneway"});
    pavedGood = c.GetTypeByPath({"psurface", "paved_good"});
    pavedBad = c.GetTypeByPath({"psurface", "paved_bad"});
    unpavedGood = c.GetTypeByPath({"psurface", "unpaved_good"});
    unpavedBad = c.GetTypeByPath({"psurface", "unpaved_bad"});
  }

  uint32_t primary, secondary, secondaryTunnel, secondaryBridge, residential;
  uint32_t oneway, pavedGood, pavedBad, unpavedGood, unpavedBad;
};

class VehicleModelStub : public VehicleModel
{
public:
  VehicleModelStub()
    : VehicleModel(classif(), kTestLimits, kCarSurface, {kDefaultSpeeds, kDefaultFactors})
  {
  }

  // We are not going to use offroad routing in these tests.
  SpeedKMpH const & GetOffroadSpeed() const override
  {
    static SpeedKMpH offroad{0.0 /* weight */, 0.0 /* eta */};
    return offroad;
  }
};

void CheckSpeedWithParams(initializer_list<uint32_t> const & types, SpeedParams const & params,
                          SpeedKMpH const & expectedSpeed)
{
  VehicleModelStub model;
  feature::TypesHolder h;
  for (uint32_t t : types)
    h.Add(t);

  TEST_EQUAL(model.GetTypeSpeed(h, params), expectedSpeed, ());
}

void CheckSpeed(initializer_list<uint32_t> const & types, InOutCitySpeedKMpH const & expectedSpeed)
{
  SpeedParams const inCity(true /* forward */, true /* in city */, Maxspeed());
  CheckSpeedWithParams(types, inCity, expectedSpeed.m_inCity);
  SpeedParams const outCity(true /* forward */, false /* in city */, Maxspeed());
  CheckSpeedWithParams(types, outCity, expectedSpeed.m_outCity);
}

void CheckOneWay(initializer_list<uint32_t> const & types, bool expectedValue)
{
  VehicleModelStub model;
  feature::TypesHolder h;
  for (uint32_t t : types)
    h.Add(t);

  TEST_EQUAL(model.HasOneWayType(h), expectedValue, ());
}

void CheckPassThroughAllowed(initializer_list<uint32_t> const & types, bool expectedValue)
{
  VehicleModelStub model;
  feature::TypesHolder h;
  for (uint32_t t : types)
    h.Add(t);

  TEST_EQUAL(model.HasPassThroughType(h), expectedValue, ());
}


UNIT_CLASS_TEST(VehicleModelStub, MaxSpeed)
{
  TEST_EQUAL(GetMaxWeightSpeed(), 150.0, ());
}

UNIT_CLASS_TEST(VehicleModelTest, Speed)
{
  {
    CheckSpeed({secondaryBridge}, kDefaultSpeeds.Get(HighwayType::HighwaySecondary));
    CheckSpeed({secondaryTunnel}, kDefaultSpeeds.Get(HighwayType::HighwaySecondary));
    CheckSpeed({secondary}, kDefaultSpeeds.Get(HighwayType::HighwaySecondary));
  }

  CheckSpeed({classif().GetTypeByPath({"highway", "trunk"})},
             {SpeedKMpH(100.0 /* weight */, 100.0 /* eta */) /* in city */,
              SpeedKMpH(150.0 /* weight */, 150.0 /* eta */) /* out of city */});
  CheckSpeed({primary}, {SpeedKMpH(90.0, 90.0), SpeedKMpH(120.0, 120.0)});
  CheckSpeed({residential}, {SpeedKMpH(22.5, 27.5), SpeedKMpH(25.0, 30.0)});
}

UNIT_CLASS_TEST(VehicleModelTest, Speed_MultiTypes)
{
  uint32_t const typeHighway = classif().GetTypeByPath({"highway"});

  CheckSpeed({secondaryTunnel, secondary}, kDefaultSpeeds.Get(HighwayType::HighwaySecondary));
  CheckSpeed({secondaryTunnel, typeHighway}, kDefaultSpeeds.Get(HighwayType::HighwaySecondary));
  CheckSpeed({typeHighway, secondaryTunnel}, kDefaultSpeeds.Get(HighwayType::HighwaySecondary));
}

UNIT_CLASS_TEST(VehicleModelTest, OneWay)
{
  CheckSpeed({secondaryBridge, oneway}, kDefaultSpeeds.Get(HighwayType::HighwaySecondary));
  CheckOneWay({secondaryBridge, oneway}, true);
  CheckSpeed({oneway, secondaryBridge}, kDefaultSpeeds.Get(HighwayType::HighwaySecondary));
  CheckOneWay({oneway, secondaryBridge}, true);

  CheckOneWay({oneway}, true);
}

UNIT_CLASS_TEST(VehicleModelTest, DifferentSpeeds)
{
  // What is the purpose of this artificial test with several highway types? To show that order is important?
  CheckSpeed({secondary, primary}, kDefaultSpeeds.Get(HighwayType::HighwaySecondary));
  CheckSpeed({oneway, primary, secondary}, kDefaultSpeeds.Get(HighwayType::HighwayPrimary));
  CheckOneWay({primary, oneway, secondary}, true);
}

UNIT_CLASS_TEST(VehicleModelTest, PassThroughAllowed)
{
  CheckPassThroughAllowed({secondary}, true);
  CheckPassThroughAllowed({primary}, true);
  CheckPassThroughAllowed({classif().GetTypeByPath({"highway", "service"})}, false);
}

UNIT_CLASS_TEST(VehicleModelTest, SpeedFactor)
{
  CheckSpeed({secondary, pavedGood},
             {SpeedKMpH(64.0 /* weight */, 63.0 /* eta */) /* in city */,
              SpeedKMpH(64.0 /* weight */, 63.0 /* eta */) /* out of city */});
  CheckSpeed({secondary, pavedBad}, {SpeedKMpH(32.0, 35.0), SpeedKMpH(32.0, 35.0)});
  CheckSpeed({secondary, unpavedGood}, {SpeedKMpH(48.0, 56.0), SpeedKMpH(48.0, 56.0)});
  CheckSpeed({secondary, unpavedBad}, {SpeedKMpH(16.0, 14.0), SpeedKMpH(16.0, 14.0)});

  CheckSpeed({residential, pavedGood}, {SpeedKMpH(18.0, 24.75), SpeedKMpH(20.0, 27.0)});
  CheckSpeed({residential, pavedBad}, {SpeedKMpH(9.0, 13.75), SpeedKMpH(10.0, 15.0)});
  CheckSpeed({residential, unpavedGood}, {SpeedKMpH(13.5, 22.0), SpeedKMpH(15.0, 24.0)});
  CheckSpeed({residential, unpavedBad}, {SpeedKMpH(4.5, 5.5), SpeedKMpH(5.0, 6.0)});
}

UNIT_CLASS_TEST(VehicleModelTest, MaxspeedFactor)
{
  Maxspeed const maxspeed90 =
      Maxspeed(measurement_utils::Units::Metric, 90 /* forward speed */, kInvalidSpeed);
  CheckSpeedWithParams({secondary, unpavedBad},
                       SpeedParams(true /* forward */, false /* in city */, maxspeed90),
                       SpeedKMpH(18.0));

  CheckSpeedWithParams({primary, pavedGood},
                       SpeedParams(true /* forward */, false /* in city */, maxspeed90),
                       SpeedKMpH(72.0, 81.0));

  Maxspeed const maxspeed9070 =
      Maxspeed(measurement_utils::Units::Metric, 90 /* forward speed */, 70);
  CheckSpeedWithParams({primary, pavedGood},
                       SpeedParams(true /* forward */, false /* in city */, maxspeed9070),
                       SpeedKMpH(72.0, 81.0));
  CheckSpeedWithParams({primary, pavedGood},
                       SpeedParams(false /* forward */, false /* in city */, maxspeed9070),
                       SpeedKMpH(56.0, 63.0));

  Maxspeed const maxspeed60 =
      Maxspeed(measurement_utils::Units::Metric, 60 /* forward speed */, kInvalidSpeed);
  CheckSpeedWithParams({residential, pavedGood},
                       SpeedParams(true /* forward */, false /* in city */, maxspeed60),
                       SpeedKMpH(24.0, 27.0));
}

namespace
{
bool LessSpeed(SpeedKMpH const & l, SpeedKMpH const & r)
{
  TEST(l.IsValid() && r.IsValid(), (l, r));
  return l.m_weight < r.m_weight && l.m_eta < r.m_eta;
}

#define TEST_LESS_SPEED(l, r) TEST(LessSpeed(l, r), (l, r))
} // namespace

UNIT_CLASS_TEST(VehicleModelTest, CarModel_TrackVsGravelTertiary)
{
  auto const & model = CarModel::AllLimitsInstance();

  auto const & c = classif();
  feature::TypesHolder h1;
  h1.Add(c.GetTypeByPath({"highway", "track"}));

  feature::TypesHolder h2;
  h2.Add(c.GetTypeByPath({"highway", "tertiary"}));
  h2.Add(unpavedBad);   // from OSM surface=gravel

  // https://www.openstreetmap.org/#map=19/45.43640/36.39689
  // Obvious that gravel tertiary (moreover with maxspeed=60kmh) should be better than track.

  {
    SpeedParams p1({}, kInvalidSpeed, false /* inCity */);
    SpeedParams p2({measurement_utils::Units::Metric, 60, 60}, kInvalidSpeed, false /* inCity */);
    TEST_LESS_SPEED(model.GetTypeSpeed(h1, p1), model.GetTypeSpeed(h2, p2));
  }

  {
    SpeedParams p({}, kInvalidSpeed, false /* inCity */);
    TEST_LESS_SPEED(model.GetTypeSpeed(h1, p), model.GetTypeSpeed(h2, p));
  }
}

#undef TEST_LESS_SPEED

UNIT_TEST(VehicleModel_MultiplicationOperatorTest)
{
  SpeedKMpH const speed(90 /* weight */, 100 /* eta */);
  SpeedFactor const factor(1.0, 1.1);
  SpeedKMpH const lResult = speed * factor;
  SpeedKMpH const rResult = factor * speed;
  TEST_EQUAL(lResult, rResult, ());
  TEST(base::AlmostEqualULPs(lResult.m_weight, 90.0), ());
  TEST(base::AlmostEqualULPs(lResult.m_eta, 110.0), ());
}

UNIT_TEST(VehicleModel_CarModelValidation)
{
  HighwayType const carRoadTypes[] = {
      HighwayType::HighwayLivingStreet,  HighwayType::HighwayMotorway,
      HighwayType::HighwayMotorwayLink,  HighwayType::HighwayPrimary,
      HighwayType::HighwayPrimaryLink,   HighwayType::HighwayResidential,
      HighwayType::HighwayRoad,          HighwayType::HighwaySecondary,
      HighwayType::HighwaySecondaryLink, HighwayType::HighwayService,
      HighwayType::HighwayTertiary,      HighwayType::HighwayTertiaryLink,
      HighwayType::HighwayTrack,         HighwayType::HighwayTrunk,
      HighwayType::HighwayTrunkLink,     HighwayType::HighwayUnclassified,
      HighwayType::ManMadePier,          HighwayType::RailwayRailMotorVehicle,
      HighwayType::RouteFerry,           HighwayType::RouteShuttleTrain,
  };

  for (auto const hwType : carRoadTypes)
  {
    auto const * factor = kHighwayBasedFactors.Find(hwType);
    TEST(factor, (hwType));
    TEST(factor->IsValid(), (hwType, *factor));

    auto const * speed = kHighwayBasedSpeeds.Find(hwType);
    TEST(speed, (hwType));
    TEST(speed->IsValid(), (hwType, *speed));
  }
}
}  // namespace vehicle_model_test
