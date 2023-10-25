#include "AABB.h"
#include <utility>

bool AABB::IsCollisionBool(const AABB& hit) {
	// 他のAABBと衝突しているかを判定
	bool collisionX = (max.x >= hit.min.x) && (min.x <= hit.max.x);
	bool collisionY = (max.y >= hit.min.y) && (min.y <= hit.max.y);
	bool collisionZ = (max.z >= hit.min.z) && (min.z <= hit.max.z);

	// 衝突している場合
	if (collisionX && collisionY && collisionZ) {
		return true;
	}
	else {
		// 衝突していない場合、ベクトル(0, 0, 0)を返します
		return false;
	}
}
Vector3 AABB::IsCollisionVector3(const AABB& hit) {
	// 衝突している場合
	if (IsCollisionBool(hit)) {
		// 衝突している場合、衝突ベクトルは各軸ごとの最小値と最大値の差を取ります
		Vector3 collisionVector = { 0.0f,0.0f,0.0f };
		if (GetCenter().x < hit.GetCenter().x) {
			collisionVector.x = std::max(min.x, hit.min.x) - std::min(max.x, hit.max.x);
		}
		else {
			collisionVector.x = std::min(max.x, hit.max.x) - std::max(min.x, hit.min.x);
		}
		if (GetCenter().z < hit.GetCenter().z) {
			collisionVector.z = std::max(min.z, hit.min.z) - std::min(max.z, hit.max.z);
		}
		else {
			collisionVector.z = std::min(max.z, hit.max.z) - std::max(min.z, hit.min.z);
		}

		// Y軸は捨てる
		collisionVector.y = 0.0f;

		// XとZのより短いベクトルのみを返す
		if (std::fabs(collisionVector.x) < std::fabs(collisionVector.z)) {
			collisionVector.z = 0.0f;
		}
		else {
			collisionVector.x = 0.0f;
		}

		// ここでcollisionVectorを使用してAABBを移動する処理を行うことができます。
		// 例えば、AABBの位置を移動するには次のようにします。
		//min =  min - collisionVector;
		//max = max - collisionVector;
		return collisionVector;
	}
	else {
		// 衝突していない場合、ベクトル(0, 0, 0)を返します
		return { 0.0f,0.0f,0.0f };
	}
}