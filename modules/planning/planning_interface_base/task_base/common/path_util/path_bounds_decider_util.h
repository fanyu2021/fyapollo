/******************************************************************************
 * Copyright 2023 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
#pragma once

#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "modules/common/util/point_factory.h"
#include "modules/planning/planning_base/common/path_boundary.h"
#include "modules/planning/planning_base/common/reference_line_info.h"
#include "modules/planning/planning_base/common/sl_polygon.h"

namespace apollo {
namespace planning {
// ObstacleEdge contains: (is_start_s, s, l_min, l_max, obstacle_id).
using ObstacleEdge = std::tuple<int, double, double, double, std::string>;
// SLSstate contains: (s ,s' ,s''), (l, l', l'')
using SLState = std::pair<std::array<double, 3>, std::array<double, 3>>;

enum class LaneBorrowInfo {
  LEFT_BORROW,
  NO_BORROW,
  RIGHT_BORROW,
};

class PathBoundsDeciderUtil {
 public:
  /**
   * @brief Starting from ADC's current position, increment until the path
   * length, and set lateral bounds to be infinite at every spot.
   */
  static bool InitPathBoundary(const ReferenceLineInfo& reference_line_info,
                               PathBoundary* const path_bound,
                               SLState init_sl_state);
  /**
   * @brief Starting from ADC's current position, increment until the horizon,
   * and and set lateral bounds to be infinite at every spot.
   */
  static void GetStartPoint(common::TrajectoryPoint planning_start_point,
                            const ReferenceLine& reference_line,
                            SLState* init_sl_state);
  /**
   * @brief Get lane width in init_sl_state.
   */
  static double GetADCLaneWidth(const ReferenceLine& reference_line,
                                const double adc_frenet_s);
  /** @brief Refine the boundary based on lane-info and ADC's location.
   *   It will comply to the lane-boundary. However, if the ADC itself
   *   is out of the given lane(s), it will adjust the boundary
   *   accordingly to include ADC's current position.
   */
  static bool GetBoundaryFromLanesAndADC(
      const ReferenceLineInfo& reference_line_info,
      const LaneBorrowInfo& lane_borrow_info, bool is_extend_adc,
      double ADC_buffer, PathBoundary* const path_bound,
      std::string* const borrow_lane_type, bool is_fallback_lanechange,
      const SLState& init_sl_state);
  /** @brief Update the path_boundary at "idx"
   *         It also checks if ADC is blocked (lmax < lmin).
   *  @param idx current index of the path_bounds
   *  @param left_bound minimum left boundary (l_max)
   *  @param right_bound maximum right boundary (l_min)
   *  @param bound_point path_boundaries (its content at idx will be updated)
   *  @return If path is good, true; if path is blocked, false.
   */
  static bool UpdatePathBoundaryWithBuffer(
      double left_bound, double right_bound, BoundType left_type,
      BoundType right_type, std::string left_id, std::string right_id,
      PathBoundPoint* const bound_point);
  /** @brief Trim the path bounds starting at the idx where path is blocked.
   */
  static bool UpdateRightPathBoundaryWithBuffer(
      double right_bound, BoundType right_type, std::string right_id,
      PathBoundPoint* const bound_point);
  static bool UpdateLeftPathBoundaryWithBuffer(
      double left_bound, BoundType left_type, std::string left_id,
      PathBoundPoint* const bound_point);
  static void TrimPathBounds(const int path_blocked_idx,
                             PathBoundary* const path_boundaries);
  /** @brief Find the farthest obstacle's id which ADC is blocked by
   *  @param obs_id_to_start_s blocked obstacles' start_s map
   *  @return If obs_id_to_start_s is empty return "", else return the nearest
   * obstacle's id.
   */
  static std::string FindFarthestBlockObstaclesId(
      const std::unordered_map<std::string, double>& obs_id_to_start_s);
  /** @brief Refine the boundary based on static obstacles. It will make sure
   *   the boundary doesn't contain any static obstacle so that the path
   *   generated by optimizer won't collide with any static obstacle.
   */
  static bool GetBoundaryFromStaticObstacles(
      std::vector<SLPolygon>* const sl_polygons, const SLState& init_sl_state,
      PathBoundary* const path_boundaries,
      std::string* const blocking_obstacle_id, double* const narrowest_width);

  static std::vector<ObstacleEdge> SortObstaclesForSweepLine(
      const IndexedList<std::string, Obstacle>& indexed_obstacles,
      const SLState& init_sl_state);
  /** @brief Update the path_boundary at "idx", as well as the new center-line.
   *         It also checks if ADC is blocked (lmax < lmin).
   *  @param idx current index of the path_bounds
   *  @param left_bound minimum left boundary (l_max)
   *  @param right_bound maximum right boundary (l_min)
   *  @param path_boundaries path_boundaries (its content at idx will be
   * updated)
   *  @param center_line center_line (to be updated)
   *  @return If path is good, true; if path is blocked, false.
   */
  static bool UpdatePathBoundaryAndCenterLineWithBuffer(
      size_t idx, double left_bound, double right_bound,
      PathBoundary* const path_boundaries, double* const center_line);
  /** @brief Get the distance between ADC's center and its edge.
   * @return The distance.
   */
  static double GetBufferBetweenADCCenterAndEdge();
  /** @brief Is obstacle should be considered in path decision
   *  @param obstacle check obstacle
   *  @return If obstacle should be considered return true
   */
  static bool IsWithinPathDeciderScopeObstacle(const Obstacle& obstacle);
  /** @brief Check is sl range has intersection with sl_boundary
   *  @param sl_boundary input sl_boundary
   *  @param s s of sl range
   * @param ptr_l_min l min pointer of sl range
   * @param ptr_l_max l max pointer of sl range
   *  @return If has intersection, return true
   */
  static bool ComputeSLBoundaryIntersection(const SLBoundary& sl_boundary,
                                            const double s, double* ptr_l_min,
                                            double* ptr_l_max);
  static common::TrajectoryPoint InferFrontAxeCenterFromRearAxeCenter(
      const common::TrajectoryPoint& traj_point);
  /** @brief generate path bound by self lane and adc position
   *  @param init_sl_state adc init sl state
   *  @param is_extend_adc_bound whether to include adc boundary in path bound
   * @param extend_buffer entend adc bound buffer
   * @param path_bound output path bound
   *  @return If succeed return true
   */
  static bool GetBoundaryFromSelfLane(
      const ReferenceLineInfo& reference_line_info,
      const SLState& init_sl_state, PathBoundary* const path_bound);

  static bool GetBoundaryFromRoad(const ReferenceLineInfo& reference_line_info,
                                  const SLState& init_sl_state,
                                  PathBoundary* const path_bound);

  /**
   * @brief extend boundary to include adc
   * @param init_sl_state adc init sl state
   * @param extend_buffer entend adc bound buffer
   * @param path_bound output path bound
   * @return
   */
  static bool ExtendBoundaryByADC(const ReferenceLineInfo& reference_line_info,
                                  const SLState& init_sl_state,
                                  const double extend_buffer,
                                  PathBoundary* const path_bound);
  static void ConvertBoundarySAxisFromLaneCenterToRefLine(
      const ReferenceLineInfo& reference_line_info,
      PathBoundary* const path_bound);
  static int IsPointWithinPathBound(
      const ReferenceLineInfo& reference_line_info, const double x,
      const double y, const PathBound& path_bound);
  static void GetSLPolygons(const ReferenceLineInfo& reference_line_info,
                            std::vector<SLPolygon>* polygons,
                            const SLState& init_sl_state);
  static bool UpdatePathBoundaryBySLPolygon(PathBoundary* path_boundary,
                                            std::vector<SLPolygon>* sl_polygon,
                                            std::vector<double>* center_s,
                                            std::string* blocked_id,
                                            double* narrowest_width);
  static bool AddCornerPoint(double s, double l_lower, double l_upper,
                             const PathBoundary& path_boundary,
                             InterPolatedPointVec* extra_constraints);
  static void AddCornerBounds(const std::vector<SLPolygon>& sl_polygons,
                              PathBoundary* path_boundary);
  static bool RelaxEgoLateralBoundary(PathBoundary* path_boundary,
                                      const SLState& init_sl_state);
};

}  // namespace planning
}  // namespace apollo
