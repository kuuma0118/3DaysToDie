#include "TitleScene.h"

void TitleScene::Initialize() {
	// タイトルの初期位置
	titleNamePos_ = { 128,100 };
	// タイトル
	titleName_ = new Sprite();
	titleName_->Initialize({ 0,0,1 }, { 1280,720,1 });
}

void TitleScene::Update() {
	// タイトルを上下に動かす
	theta_ += 1.0f / 90.0f * (float)M_PI;
	titleNamePos_.y += sinf(theta_) * 0.25f;
}

void TitleScene::Draw() {
	titleName_->Draw({ 0,0,0 }, TITLENAME);
}

void TitleScene::Finalize() {
	delete titleName_;
}