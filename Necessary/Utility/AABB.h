#pragma once
#include "../Blossom Engine/Math/Vector3.h"

class AABB {
public:
	Vector3 min; //!< 最小点
	Vector3 max; //!< 最大点

	// AABB * AABB
	bool IsCollisionBool(const AABB& hit);
	Vector3 IsCollisionVector3(const AABB& hit);

	// AABBの中心点を求める関数
	Vector3 GetCenter() const {
		return (min + max) * 0.5f;
	}
};