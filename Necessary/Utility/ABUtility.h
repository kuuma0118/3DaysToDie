#pragma once
#include "../Blossom Engine/Math/Vector3.h"

namespace Utility {
	// 線形補完
	Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t) {
		return v1 + ((v2 - v1) * t);
	}
	// 球面線形補間
	Vector3 Slerp(const Vector3& v1, const Vector3& v2, float t) {
		return v1 * (1 - t) + v2 * t;
	}
}