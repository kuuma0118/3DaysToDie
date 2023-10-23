#pragma once
#include "IScene.h"
#include "../Blossom Engine/Sprite/Sprite.h"
#include "../Blossom Engine/Sphere/Sphere.h"

class GameManager;

class TitleScene : public IScene
{
public:
	void Initialize() override;
	void Update() override;
	void Draw() override;
	void Finalize()override;
private:
	Sprite* sprite_;
	Sphere* sphere_;
	int textureNum_;
	/*Input* input_;*/
	Vector3 pos_;

};