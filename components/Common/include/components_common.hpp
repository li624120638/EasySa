#ifndef COMPONENTS_COMMON_HPP_
#define COMPONENTS_COMMON_HPP_
#include <vector>
#include <string>

namespace easysa {
	namespace components {
		typedef struct BoundingBox {
			float x; // top left coordinates x
			float y; // top left coordinates y
			float width;
			float height;
		} BoundingBox;

		typedef struct DetectObject {
			std::string label;
			float score;
			BoundingBox bbox;
			int track_id;
			//  track plugins will use this attribute.
			int age = 0;
			// @param use to store object Features extraction result
			std::vector<float> feature;
		} DetectObject;

		float GetBBoxIOU(const BoundingBox& A, const BoundingBox& B) {
			float cross_area = (A.x + A.width - B.x) * (B.y - A.y - A.height);
			if (cross_area <= 0) {
				return 0;
			}
			float area = (A.width * A.height) + (B.width * B.height) - cross_area;
			if (area <= 0) {
				return 0;
			}
			return (cross_area / area);
		}
	} // namespace components
} // namespace easysa

#endif // !COMPONENTS_COMMON_HPP_
