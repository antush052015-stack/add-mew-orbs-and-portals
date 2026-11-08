#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "state.hpp"
#include <unordered_map>
#include <array>

using namespace geode::prelude;

// ---------------------------------------------------------------
// 30 НОВЫХ ПОРТАЛОВ СКОРОСТИ
// ID 91001..91030 — заглушки. Чтобы порталы появились в редакторе,
// нужны спрайты и регистрация объектов (см. README).
// Множитель считается относительно обычной скорости (1.0 = 1x).
// ---------------------------------------------------------------
namespace sp {
	constexpr int FIRST_ID = 91001;

	constexpr std::array<float, 30> SPEEDS = {
		0.25f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f,
		1.1f, 1.25f, 1.4f, 1.5f, 1.75f,
		2.0f, 2.25f, 2.5f, 2.75f,
		3.0f, 3.25f, 3.5f, 3.75f,
		4.0f, 4.5f,
		5.0f, 5.5f,
		6.0f, 6.5f,
		7.0f, 7.5f,
		8.0f
	};

	inline const std::unordered_map<int, float>& table() {
		static const std::unordered_map<int, float> m = [] {
			std::unordered_map<int, float> t;
			for (size_t i = 0; i < SPEEDS.size(); ++i) {
				t[FIRST_ID + static_cast<int>(i)] = SPEEDS[i];
			}
			return t;
		}();
		return m;
	}
}

class $modify(SpeedPortalPlayer, PlayerObject) {
	// Срабатывает при касании любого объекта; нас интересуют только наши порталы.
	// Если сигнатура в твоей версии биндингов отличается — поправь по заголовку PlayerObject.hpp.
	void collidedWithObject(float dt, GameObject* obj, cocos2d::CCRect rect, bool anyFlag) {
		PlayerObject::collidedWithObject(dt, obj, rect, anyFlag);
		if (!obj) return;

		// Портал нового режима включает «Стрелочный полёт»
		if (obj->m_objectID == st::ARROW_PORTAL_ID) {
			st::arrowMode = true;
			return;
		}

		// Портал любой другой формы выключает его
		switch (obj->m_objectType) {
			case GameObjectType::CubePortal:
			case GameObjectType::ShipPortal:
			case GameObjectType::BallPortal:
			case GameObjectType::UfoPortal:
			case GameObjectType::WavePortal:
			case GameObjectType::RobotPortal:
			case GameObjectType::SpiderPortal:
			case GameObjectType::SwingPortal:
				st::arrowMode = false;
				break;
			default: break;
		}

		auto it = sp::table().find(obj->m_objectID);
		if (it != sp::table().end()) {
			this->m_playerSpeed = it->second;
		}
	}
};
