#ifndef PLUGINS_TRACK_HPP_
#define PLUGINS_TRACK_HPP_
#include "track_base.hpp"
#include <list>
#include <memory>
/*
*	Notice: To holdover performance
*	nn_budget maybe little. 
*	implement this function, i will use bbox ,iou.
*/
namespace easysa {
	namespace plugins {
		class FeatureMatchTrack : public TrackBase {
		public:
			FeatureMatchTrack();
			~FeatureMatchTrack();
			/*
			* @brief Set parmas related to Tracking algorithm
			* @param max_cosine_distance Threshlod of cosine distance
			* @param nn_budget Tracker only save the latest [nn_budget]
			* samples of feature for each object
			* @param max_iou_distance Threshold of iou distance
			* @param max_age Object stay alive for [max_age] after disappeare
			* @param n_init After matched [n_init] times in a row, object is
			* turned from Tentative to Confirmed
			*/
			void SetParams(float max_cosine_distance, int nn_budget, float max_iou_distance,
				int max_age, int n_init);
			/*
			* @brief Update object status and do tracking using cascade matching
			* and iou matching
			* @param frame Track frame
			* @param dectects Objects(std::vector<DectectObject>)
			* @param tracks Tracked objects
			*/
			void UpdateTrackStatus(const TrackFrame& frame, Objects& detects) override;
		private:
			int max_age_ = 20;
			int n_init_ = 3;
			uint32_t nn_budget_ = 40;
			float max_cosine_distance_ = 0.2;
			float max_iou_distance_ = 0.7;
			uint32_t total_track_num_ = 0;
			std::list<std::shared_ptr<DetectObject>> built_in_obj_;
		}; // class FeatureMatchTrack
	} // namespace easysa
} // namespace easysa
#endif // !PLUGINS_TRACK_HPP_
