#include "Map.h"
#include "../ImGui/ImGuiManager.h"
#include "../Blossom Engine/components/Input/Input.h"
#include "../Blossom Engine/components/camera/DebugCamera.h"
#include "../Models/Block.h"
#include <unordered_set>


void Map::Initialize() {
	// スケールを調整
	kBlockScale_ = std::min<float>({ 5.0f / GetXSize(), 5.0f / GetYSize(), 5.0f / GetZSize() });

	// 地面のブロックを生成
	AddGroundBlock();

	// マップチップを参照し、必要なブロックを生成
	for (int x = 0; x < GetXSize(); x++) {
		for (int y = 0; y < GetYSize(); y++) {
			for (int z = 0; z < GetZSize(); z++) {
				switch (mapChip_[x][y][z].type)
				{
				case BlockTypeID::Air:
				case BlockTypeID::OutOfArea:
					break;
				case BlockTypeID::StartPosition:
					startPosition_ = GetWorldPosition(x, y, z);
					mapChip_[x][y][z].type = BlockTypeID::Air;
					break;
				case BlockTypeID::Wall: {
					Block* newBlock = new Block();
					newBlock->Initialize();
					newBlock->transform.translate = GetWorldPosition(x, y, z);
					newBlock->transform.scale = { kBlockScale_,kBlockScale_,kBlockScale_ };
					newBlock->textureNum = GROUND;
					wallBlocks_.push_back(newBlock);
					break; }
				case BlockTypeID::Wall2: {
					Block* newBlock = new Block();
					newBlock->Initialize();
					newBlock->transform.translate = GetWorldPosition(x, y, z);
					newBlock->transform.scale = { kBlockScale_,kBlockScale_,kBlockScale_ };
					newBlock->textureNum = WALL;
					wallBlocks_.push_back(newBlock);
					break; }
				default:
					mapChip_[x][y][z].blockPtr = new Block();
					mapChip_[x][y][z].blockPtr->Initialize();
					mapChip_[x][y][z].blockPtr->transform.translate = GetWorldPosition(x, y, z);
					mapChip_[x][y][z].blockPtr->transform.scale = { kBlockScale_,kBlockScale_,kBlockScale_ };
					mapChip_[x][y][z].blockPtr->textureNum = DEFAULT_BLOCK + (((int)mapChip_[x][y][z].type - 1) % 10) + 1;
					break;
				}
			}
		}
	}

}
void Map::Update() {
	// ブロック消滅処理
	CheckClearBlock();

	// プレイヤーがブロックをつかんでいる間。移動処理はマップが扱う
	if (player_ != nullptr) {
		// 移動クールタイム
		if (blockMoveCoolTime > 0) {
			blockMoveCoolTime--;
			return;
		}
		// 移動処理
		XINPUT_STATE joyState;

		// ゲームパッド状態取得
		if (Input::GetInstance()->GetJoystickState(0, joyState)) {

			// デッドゾーンの設定
			SHORT leftThumbX = Input::GetInstance()->ApplyDeadzone(joyState.Gamepad.sThumbLX);
			SHORT leftThumbY = Input::GetInstance()->ApplyDeadzone(joyState.Gamepad.sThumbLY);

			// プレイヤー移動
			Vector3 move = {
				(float)leftThumbX / SHRT_MAX,
				0.0f,
				(float)leftThumbY / SHRT_MAX
			};
			Vector3 rotation = DebugCamera::GetInstance()->GetCameraRotation();
			rotation.x = 0.0f;	// X軸は捨てる
			move = move * MakeRotateMatrix(rotation);
			move.y = 0.0f;	// Y軸は捨てる
			// 0.75以上の傾きでなければ検出しない
			bool moved = false;
			if (move.x < -0.75f) {	// 左移動
				moved = MoveOnMapChip({ -1.0f,0.0f,0.0f }, grabbedBlockID, true);
			}
			else if (move.x > 0.75f) {	// 右移動
				moved = MoveOnMapChip({ 1.0f,0.0f,0.0f }, grabbedBlockID, true);
			}
			else if (move.z < -0.75f) {	// 手前移動
				moved = MoveOnMapChip({ 0.0f,0.0f,-1.0f }, grabbedBlockID, true);
			}
			else if (move.z > 0.75f) {	// 奥移動
				moved = MoveOnMapChip({ 0.0f,0.0f,1.0f }, grabbedBlockID, true);
			}

			if (moved) {
				blockMoveCoolTime = kBlockMoveCoolTime_;
			}
		}
	}

	// 落下処理
	FallBlock();
}
void Map::Draw() {
	for (Block* block : groundBlocks_) {
		block->Draw();
	}
	for (int x = 0; x < GetXSize(); x++) {
		for (int y = 0; y < GetYSize(); y++) {
			for (int z = 0; z < GetZSize(); z++) {
				if (mapChip_[x][y][z].blockPtr != nullptr) {
					mapChip_[x][y][z].blockPtr->Draw();
				}
			}
		}
	}
	for (Block* block : wallBlocks_) {
		block->Draw();
	}
}

bool Map::CheckClear() {
	for (int x = 0; x < GetXSize(); x++) {
		for (int y = 0; y < GetYSize(); y++) {
			for (int z = 0; z < GetZSize(); z++) {
				switch (mapChip_[x][y][z].type)
				{
				case BlockTypeID::Air:
				case BlockTypeID::Wall:
				case BlockTypeID::StartPosition:
				case BlockTypeID::OutOfArea:
					break;
				default:
					return false;
				}
			}
		}
	}

	return true;
}

void Map::ClearMapChip() {
	for (int x = 0; x < GetXSize(); x++) {
		for (int y = 0; y < GetYSize(); y++) {
			for (int z = 0; z < GetZSize(); z++) {
				mapChip_[x][y][z].type = BlockTypeID::OutOfArea;	// エリア外で初期化
			}
		}
	}
}
void Map::SetMapChip(Vector3 position, BlockTypeID id) {
	int x = static_cast<int>(position.x);
	int y = static_cast<int>(position.y);
	int z = static_cast<int>(position.z);

	// x、y、zが現在のmapChip_のサイズを超えている場合、拡張する
	while (x >= mapChip_.size()) {
		mapChip_.push_back(std::vector<std::vector<MapChip>>()); // 新しいx層を追加
	}
	while (y >= mapChip_[x].size()) {
		mapChip_[x].push_back(std::vector<MapChip>()); // 新しいy層を追加
	}
	while (z >= mapChip_[x][y].size()) {
		mapChip_[x][y].push_back(MapChip()); // 新しいz層を追加
	}

	// 指定した位置にidを設定
	mapChip_[x][y][z].type = id;
	mapChip_[x][y][z].blockPtr = nullptr;
	// ブロックのIDならば最大値を保持
	if ((int)id >= 1 && (int)id <= 98) {
		if (currentMaxID < (int)id) { currentMaxID = (int)id; }
	}
}
void Map::ReverseZ() {
	for (std::vector<std::vector<MapChip>>& xRow : mapChip_) {
		for (std::vector<MapChip>& yRow : xRow) {
			std::reverse(yRow.begin(), yRow.end()); // Z軸を逆順に並び替え
		}
	}
}
Vector3 Map::GetPlayerStartPosition() {
	return startPosition_;
}

bool Map::CheckPositionIsBlock(Vector3 position) {
	Vector3 underPos = GetMapChipPosition(position);
	underPos.y -= 1.0f;

	// 場外ならtrueを返す
	if (CheckOutOfArea(underPos)) { return true; }
	return false;
}

size_t Map::GetXSize() {
	if (mapChip_.empty()) {
		// エラー処理: マップが空の場合、Xのサイズは不明です。
		return 1;
	}
	return mapChip_.size();
}
size_t Map::GetYSize() {
	if (mapChip_.empty() || mapChip_[0].empty()) {
		// エラー処理: マップが空の場合、Yのサイズは不明です。
		return 0;
	}
	return mapChip_[0].size();
}
size_t Map::GetZSize() {
	if (mapChip_.empty() || mapChip_[0].empty() || mapChip_[0][0].empty()) {
		// エラー処理: マップが空の場合、Zのサイズは不明です。
		return 0;
	}
	return mapChip_[0][0].size();
}

Vector3 Map::GetWorldPosition(int x, int y, int z) {
	return { x * kBlockSize_ * kBlockScale_, y * kBlockSize_ * kBlockScale_, z * kBlockSize_ * kBlockScale_ };
}
Vector3 Map::GetMapChipPosition(Vector3 worldPosition) {
	// ワールド座標から配列のX、Y、Z座標を計算
	int x = static_cast<int>(std::round(worldPosition.x / (kBlockSize_ * kBlockScale_)));
	int y = static_cast<int>(std::round(worldPosition.y / (kBlockSize_ * kBlockScale_)));
	int z = static_cast<int>(std::round(worldPosition.z / (kBlockSize_ * kBlockScale_)));

	return Vector3((float)x, (float)y, (float)z);
}
void Map::AddGroundBlock() {
	for (int x = 0; x < GetXSize(); x++) {
		for (int z = 0; z < GetZSize(); z++) {
			Block* newBlock = new Block();
			newBlock->Initialize();
			newBlock->transform.translate = GetWorldPosition(x, -1, z);
			newBlock->transform.scale = { kBlockScale_,kBlockScale_, kBlockScale_ };
			newBlock->textureNum = GROUND;
			groundBlocks_.push_back(newBlock);
		}
	}
}

Vector3 Map::GetMapCenterPosition() {
	Vector3 centerPos = GetWorldPosition((int)GetXSize(), (int)GetYSize(), (int)GetZSize());
	centerPos.x /= 2.0f;
	centerPos.x -= (kBlockSize_ * kBlockScale_) / 2.0f;
	centerPos.y /= 2.0f;
	centerPos.y -= (kBlockSize_ * kBlockScale_);
	centerPos.z /= 2.0f;
	centerPos.z -= (kBlockSize_ * kBlockScale_) / 2.0f;
	return centerPos;
}

Vector3 Map::IsCollisionVector3(AABB aabb) {
	// 最終的に返すベクトル
	Vector3 result = { 0.0f,0.0f,0.0f };
	float limitX = (GetXSize() * (kBlockSize_ * kBlockScale_)) - (kBlockSize_ / 2.0f * kBlockScale_);
	float limitZ = (GetZSize() * (kBlockSize_ * kBlockScale_)) - (kBlockSize_ / 2.0f * kBlockScale_);

	// 場外に出ていないかチェック
	if (aabb.min.x < -1.0f) {
		result.x -= 1.0f + aabb.min.x;
	}
	else if (aabb.max.x > limitX) {
		result.x += limitX - aabb.max.x;
	}
	if (aabb.min.z < -1.0f) {
		result.z -= 1.0f + aabb.min.z;
	}
	else if (aabb.max.z > limitX) {
		result.z += limitX - aabb.max.z;
	}


	// 全ブロックとの当たり判定を取る
	for (int x = 0; x < GetXSize(); x++) {
		for (int y = 0; y < GetYSize(); y++) {
			for (int z = 0; z < GetZSize(); z++) {
				if (mapChip_[x][y][z].blockPtr != nullptr) {
					// ブロックからAABBを計算する
					AABB collision;
					Vector3 blockSize = { (kBlockSize_ / 2.0f) * kBlockScale_ ,(kBlockSize_ / 2.0f) * kBlockScale_ , (kBlockSize_ / 2.0f) * kBlockScale_ };
					collision.min = mapChip_[x][y][z].blockPtr->transform.translate - blockSize;
					collision.max = mapChip_[x][y][z].blockPtr->transform.translate + blockSize;
					// 当たり判定をチェック
					Vector3 fixVector = aabb.IsCollisionVector3(collision);
					// 計算するAABBを修正してから、当たり判定に戻る
					aabb.min += fixVector;
					aabb.max += fixVector;
					result += fixVector;
				}
				else if (mapChip_[x][y][z].type == BlockTypeID::Wall) {
					// ブロックからAABBを計算する
					AABB collision;
					Vector3 blockSize = { (kBlockSize_ / 2.0f) * kBlockScale_ ,(kBlockSize_ / 2.0f) * kBlockScale_ , (kBlockSize_ / 2.0f) * kBlockScale_ };
					collision.min = GetWorldPosition(x, y, z) - blockSize;
					collision.max = GetWorldPosition(x, y, z) + blockSize;
					// 当たり判定をチェック
					Vector3 fixVector = aabb.IsCollisionVector3(collision);
					// 計算するAABBを修正してから、当たり判定に戻る
					aabb.min += fixVector;
					aabb.max += fixVector;
					result += fixVector;
				}
			}
		}
	}

	return result;
}
bool Map::IsCollisionBool(Vector3 position) {
	Vector3 mapChipPos = GetMapChipPosition(position);
	// 場外チェック
	if (CheckOutOfArea(mapChipPos)) { return true; }

	int id = (int)mapChip_[(int)mapChipPos.x][(int)mapChipPos.y][(int)mapChipPos.z].type;
	if (id >= 1 && id <= 99) { return true; }

	return false;
}


bool Map::GrabBlock(Player* player) {
	Vector3 grabPos = GetMapChipPosition(player->transform.translate);	// つかむ座標を計算する
	Vector3 r{ 0.0f,0.0f,0.0f };
	// 向きからつかむ方向を出す
	if (player->transform.rotate.y > -0.785f && player->transform.rotate.y < 0.785f) {	// 奥向き
		grabPos.z++;
		r = { 0.0f,0.0f,0.0f };
	}
	else if (player->transform.rotate.y > 0.785f && player->transform.rotate.y < 2.355f) {	// 右向き
		grabPos.x++;
		r = { 0.0f,1.57f,0.0f };
	}
	else if (player->transform.rotate.y > 2.355f || player->transform.rotate.y < -2.355f) {	// 手前向き
		grabPos.z--;
		r = { 0.0f,3.14f,0.0f };
	}
	else if (player->transform.rotate.y > -2.355f && player->transform.rotate.y < -0.785f) {	// 左向き
		grabPos.x--;
		r = { 0.0f,-1.57f,0.0f };
	}

	// つかんだ位置が場外ならfalseを返す
	if (CheckOutOfArea(grabPos)) {
		return false;
	}
	// つかんだ位置にブロックがないならばfalseを返す
	if ((int)GetMapChip(grabPos) < 1 || (int)GetMapChip(grabPos) > 98) {
		return false;
	}

	// プレイヤーをマップチップにセット
	Vector3 playerPos = GetMapChipPosition(player->transform.translate);

	// 掴んだ位置とプレイヤーの足元のブロックIDが同じならばfalseを返す
	if (!CheckOutOfArea({ playerPos.x,playerPos.y - 1.0f,playerPos.z })) {
		if ((int)GetMapChip(grabPos) == (int)GetMapChip({ playerPos.x,playerPos.y - 1.0f,playerPos.z })) {
			return false;
		}
	}

	mapChip_[(int)playerPos.x][(int)playerPos.y][(int)playerPos.z].type = BlockTypeID::Player;
	// プレイヤーのポインタをセット
	player_ = player;
	// プレイヤーのつかんだブロックの座標
	grabPosition_ = grabPos;
	// プレイヤーのつかんだブロックID
	grabbedBlockID = (int)mapChip_[(int)grabPos.x][(int)grabPos.y][(int)grabPos.z].type;

	// プレイヤーの座標をマップチップに合わせる
	player->transform.translate = GetWorldPosition((int)playerPos.x, (int)playerPos.y, (int)playerPos.z);
	// プレイヤーの向きをブロックのほうに合わせる
	player->transform.rotate = r;

	// ブロックムーブクールタイム
	blockMoveCoolTime = kBlockMoveCoolTime_;

	return true;
}
void Map::ReleaseBlock() {
	player_ = nullptr;
	grabbedBlockID = -1;
	// マップチップからプレイヤーを削除
	for (int x = 0; x < GetXSize(); x++) {
		for (int y = 0; y < GetYSize(); y++) {
			for (int z = 0; z < GetZSize(); z++) {
				if (mapChip_[x][y][z].type == BlockTypeID::Player) {
					mapChip_[x][y][z].type = BlockTypeID::Air;
				}
			}
		}
	}
}

bool Map::MoveOnMapChip(Vector3 destination, int moveBlockID, bool moveWithPlayer) {
	int sizeX = (int)GetXSize();
	int sizeY = (int)GetYSize();
	int sizeZ = (int)GetZSize();

	std::vector<std::vector<std::vector<MapChip>>> newMapChip = mapChip_;// 新しいマップチップ
	Vector3 newPlayerMapChipPosition;

	if (moveWithPlayer) {
		// まずはプレイヤーが移動できるかを検証
		newPlayerMapChipPosition = GetMapChipPosition(player_->transform.translate);
		// プレイヤーの元の座標は消す
		newMapChip[(int)newPlayerMapChipPosition.x][(int)newPlayerMapChipPosition.y][(int)newPlayerMapChipPosition.z].type = BlockTypeID::Air;
		// プレイヤーの移動先
		newPlayerMapChipPosition += destination;
		// 場外検知
		if (CheckOutOfArea(newPlayerMapChipPosition)) {
			return false;
		}
		// 移動先が移動可能なマップチップかどうか
		if (GetMapChip(newPlayerMapChipPosition) != BlockTypeID::Air && (int)GetMapChip(newPlayerMapChipPosition) != moveBlockID) {
			return false;
		}
		// 移動先の下が足場かどうか
		if (!CheckOutOfArea(newPlayerMapChipPosition + Vector3{ 0.0f,-1.0f,0.0f })) {
			int underID = (int)GetMapChip(newPlayerMapChipPosition + Vector3{ 0.0f,-1.0f,0.0f });
			if (underID < 1 || underID > 99) {
				return false;
			}
		}
	}

	// まず新しいマップチップから動かすブロックと同じIDを全消去
	for (int x = 0; x < sizeX; x++) {
		for (int y = 0; y < sizeY; y++) {
			for (int z = 0; z < sizeZ; z++) {
				if ((int)newMapChip[x][y][z].type == moveBlockID) {
					newMapChip[x][y][z].type = BlockTypeID::Air;
					newMapChip[x][y][z].blockPtr = nullptr;
				}
			}
		}
	}
	// 次に動かすブロック全部の座標を取得
	for (int x = 0; x < sizeX; x++) {
		for (int y = 0; y < sizeY; y++) {
			for (int z = 0; z < sizeZ; z++) {
				if ((int)mapChip_[x][y][z].type == moveBlockID) {
					Vector3 nextPos = { (float)x, (float)y, (float)z };
					nextPos += destination;
					// ブロックが１つでも場外にでるなら移動不可なのでreturn
					if (CheckOutOfArea(nextPos)) {
						return false;
					}
					// 移動先に問題がないかチェック
					if (GetMapChip(nextPos) == BlockTypeID::Air || GetMapChip(nextPos) == BlockTypeID::Player || (int)GetMapChip(nextPos) == moveBlockID) {
						newMapChip[(int)nextPos.x][(int)nextPos.y][(int)nextPos.z].type = mapChip_[x][y][z].type;
						newMapChip[(int)nextPos.x][(int)nextPos.y][(int)nextPos.z].blockPtr = mapChip_[x][y][z].blockPtr;
					}
					else {
						return false;
					}
				}
			}
		}
	}
	// 全てのチェックを超えたので移動処理

	// プレイヤーの位置を修正
	if (moveWithPlayer) {
		newMapChip[(int)newPlayerMapChipPosition.x][(int)newPlayerMapChipPosition.y][(int)newPlayerMapChipPosition.z].type = BlockTypeID::Player;
		newMapChip[(int)newPlayerMapChipPosition.x][(int)newPlayerMapChipPosition.y][(int)newPlayerMapChipPosition.z].blockPtr = nullptr;
		player_->transform.translate = GetWorldPosition((int)newPlayerMapChipPosition.x, (int)newPlayerMapChipPosition.y, (int)newPlayerMapChipPosition.z);
		// つかんでいるブロックの位置も修正
		grabPosition_ += destination;
	}
	// ブロックの位置を修正
	for (int x = 0; x < sizeX; x++) {
		for (int y = 0; y < sizeY; y++) {
			for (int z = 0; z < sizeZ; z++) {
				if (newMapChip[x][y][z].blockPtr != nullptr)
					newMapChip[x][y][z].blockPtr->transform.translate = GetWorldPosition((int)x, (int)y, (int)z);
			}
		}
	}

	// マップチップを更新
	mapChip_ = newMapChip;
	return true;
}
void Map::FallBlock() {
	// 存在するブロックの全IDを取得
	std::vector<int> ids;
	bool isFelled = false;

	do {
		for (int x = 0; x < GetXSize(); x++) {
			for (int y = 0; y < GetYSize(); y++) {
				for (int z = 0; z < GetZSize(); z++) {
					if ((int)mapChip_[x][y][z].type >= 1 && (int)mapChip_[x][y][z].type <= 98) {
						if (std::find(ids.begin(), ids.end(), (int)mapChip_[x][y][z].type) == ids.end()) {
							ids.push_back((int)mapChip_[x][y][z].type);
						}
					}
				}
			}
		}

		// 全IDの落下処理を行う
		for (int id : ids) {
			isFelled = MoveOnMapChip({ 0.0f,-1.0f,0.0f }, id, false);
		}
	} while (isFelled);
}
void Map::CheckClearBlock() {
	// 5個以上並ぶブロックの集まりを調べる
	const int minSequenceLength = 5; // 5個以上の連続を検出するための最小長

	// サイズ
	int sizeX = (int)GetXSize();
	int sizeY = (int)GetYSize();
	int sizeZ = (int)GetZSize();

	// 最終的に消滅する全ブロックの座標
	std::vector<Vector3> deleteBlocks;
	std::vector<int> deleteBlockIDs;

	// X軸方向の検出
	for (int x = 0; x < sizeX; x++) {
		for (int y = 0; y < sizeY; y++) {
			for (int z = 0; z < sizeZ; z++) {
				if (mapChip_[x][y][z].blockPtr == nullptr)
					continue;
				int consecutiveCount = 1;

				// 右方向に検出
				for (int i = x + 1; i < sizeX; i++) {
					if (mapChip_[i][y][z].blockPtr != nullptr) {
						consecutiveCount++;
					}
					else {
						break;
					}
				}

				// 5個以上並んでいるかチェック
				if (consecutiveCount >= minSequenceLength) {
					// 並んでいる座標全てをvectorに追加
					for (int i = 0; i < consecutiveCount; i++) {
						deleteBlocks.push_back(Vector3{ static_cast<float>(x + i),static_cast<float>(y),static_cast<float>(z) });
						deleteBlockIDs.push_back((int)mapChip_[x + i][y][z].type);
					}
				}
			}
		}
	}

	// Y軸方向の検出（X軸と同様の方法で）
	for (int y = 0; y < sizeY; y++) {
		for (int x = 0; x < sizeX; x++) {
			for (int z = 0; z < sizeZ; z++) {
				if (mapChip_[x][y][z].blockPtr == nullptr)
					continue;
				int consecutiveCount = 1;

				// 上方向に検出
				for (int j = y + 1; j < sizeY; j++) {
					if (mapChip_[x][j][z].blockPtr != nullptr) {
						consecutiveCount++;
					}
					else {
						break;
					}
				}

				// 5個以上並んでいるかチェック
				if (consecutiveCount >= minSequenceLength) {
					// 並んでいる座標全てをvectorに追加
					for (int i = 0; i < consecutiveCount; i++) {
						deleteBlocks.push_back(Vector3{ static_cast<float>(x),static_cast<float>(y + i),static_cast<float>(z) });
						deleteBlockIDs.push_back((int)mapChip_[x][y + i][z].type);
					}
				}
			}
		}
	}

	// Z軸方向の検出（X軸と同様の方法で）
	for (int z = 0; z < sizeZ; z++) {
		for (int x = 0; x < sizeX; x++) {
			for (int y = 0; y < sizeY; y++) {
				if (mapChip_[x][y][z].blockPtr == nullptr)
					continue;
				int consecutiveCount = 1;

				// 前方向に検出
				for (int k = z + 1; k < sizeZ; k++) {
					if (mapChip_[x][y][k].blockPtr != nullptr) {
						consecutiveCount++;
					}
					else {
						break;
					}
				}

				// 5個以上並んでいるかチェック
				if (consecutiveCount >= minSequenceLength) {
					// 並んでいる座標全てをvectorに追加
					for (int i = 0; i < consecutiveCount; i++) {
						deleteBlocks.push_back(Vector3{ static_cast<float>(x),static_cast<float>(y),static_cast<float>(z + i) });
						deleteBlockIDs.push_back((int)mapChip_[x][y][z + i].type);
					}
				}
			}
		}
	}

	// 全ブロック削除
	for (Vector3 vector : deleteBlocks) {
		mapChip_[(int)vector.x][(int)vector.y][(int)vector.z].type = BlockTypeID::Air;
		delete mapChip_[(int)vector.x][(int)vector.y][(int)vector.z].blockPtr;
		mapChip_[(int)vector.x][(int)vector.y][(int)vector.z].blockPtr = nullptr;
	}

	// 削除するブロックIDの重複を削除
	std::unordered_set<int> uniqueSet(deleteBlockIDs.begin(), deleteBlockIDs.end());
	deleteBlockIDs.assign(uniqueSet.begin(), uniqueSet.end());
	// 削除したブロックのIDが、まだ残っているかチェック
	for (int id : uniqueSet) {
		AssignIDs(id);
	}

	// もしつかんでいるブロックが消えたならつかみ解除
	if (player_ != nullptr) {
		if (mapChip_[(int)grabPosition_.x][(int)grabPosition_.y][(int)grabPosition_.z].blockPtr == nullptr) {
			player_->ReleaseGrab();
		}
	}
}

bool Map::CheckOutOfArea(Vector3 position) {
	if (position.x < 0.0f || position.x >= GetXSize() ||
		position.y < 0.0f || position.y >= GetYSize() ||
		position.z < 0.0f || position.z >= GetZSize()) {
		return true;
	}
	return false;
}




void Map::FloodFill(int x, int y, int z, int newID, int oldID) {
	if (CheckOutOfArea({ (float)x, (float)y, (float)z }) || (int)mapChip_[x][y][z].type != oldID) {
		return;
	}
	// 上、下、左、右、前、後、の移動方向
	int dx[] = { -1, 1, 0, 0, 0, 0 };
	int dy[] = { 0, 0, -1, 1, 0, 0 };
	int dz[] = { 0, 0, 0, 0, -1, 1 };

	mapChip_[x][y][z].type = (BlockTypeID)newID;
	mapChip_[x][y][z].blockPtr->textureNum = DEFAULT_BLOCK + (((int)mapChip_[x][y][z].type - 1) % 10) + 1;
	for (int i = 0; i < 6; ++i) {
		FloodFill(x + dx[i], y + dy[i], z + dz[i], newID, oldID);
	}
}
void Map::AssignIDs(int checkID) {
	int sizeX = (int)GetXSize();
	int sizeY = (int)GetYSize();
	int sizeZ = (int)GetZSize();
	//std::vector<std::vector<std::vector<bool>>> visited(sizeX, std::vector<std::vector<bool>>(sizeY, std::vector<bool>(sizeZ, false)));	// 訪問済みフラグ

	for (int x = 0; x < sizeX; x++) {
		for (int y = 0; y < sizeY; y++) {
			for (int z = 0; z < sizeZ; z++) {
				if ((int)mapChip_[x][y][z].type == checkID) {
					currentMaxID++;
					if (currentMaxID >= 99) { currentMaxID = 0; }
					FloodFill(x, y, z, currentMaxID, checkID);
				}
			}
		}
	}
}
