#include "track.hpp"

namespace easysa {
	namespace plugins {
		FeatureMatchTrack::FeatureMatchTrack() {
		}
		FeatureMatchTrack::~FeatureMatchTrack() {
		}
		void FeatureMatchTrack::SetParams(float max_cosine_distance, int nn_budget,
			float max_iou_distance, int max_age, int n_init) {
			max_cosine_distance_ = max_cosine_distance;
			nn_budget_ = nn_budget;
			max_cosine_distance_ = max_cosine_distance;
			max_age_ = max_age;
			n_init_ = n_init;
			built_in_obj_.resize(nn_budget);
		}

		void FeatureMatchTrack::UpdateTrackStatus(const TrackFrame& frame, Objects& detects) {
			/*
			*	use tracks to update detects, and update tracks
			*/
			size_t sz = detects.size();
			for (auto iter = built_in_obj_.begin(); iter != built_in_obj_.end(); ++iter) {
				float max_iou = 0;
				int max_index = 0;
				for (int i = 0; i < sz; ++i) {
					float iou = GetBBoxIOU(*iter->get(), it);
					if (iou > max_iou) {
						max_iou = iou;
						max_index = i;
					}
				}
				/*
				* if track sucess
				*/
				if (max_iou > max_iou_distance_) {
					detects[max_index].track_id = iter->get()->track_id;
					detects[max_index].label = iter->get()->label;
					detects[max_index].feature = iter->get()->feature;
					detects[max_index].age = iter->get()->age;
					// set iter age
					iter->get()->age = 0;
					iter->get()->bbox = detects[max_index].bbox;
				}else {
					iter->get()->age += 1;
					if (iter->get()->age > max_age_) {
						built_in_obj_.erase(iter);
					}
				}

				for (auto& it : detects) {
					if (it.track_id < 0) {
						if (built_in_obj_.size() < nn_budget_) {
							std::shared_ptr<DetectObject> obj = std::make_shared<DetectObject>();
							// to recognize, get label and feature
							obj->bbox = it.bbox;
							obj->age = 0;
							obj->track_id = it.track_id;
							obj->score = it.score;
							total_track_num_ += 1;
							obj->track_id = total_track_num_;
							built_in_obj_.push_back(obj);
						}
					}
				}
			}
		}
	} // namespace plugins
} // namespace easysa