#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <functional>
#include <unordered_map>

using namespace geode::prelude;

#include "state.hpp"

// ---------------------------------------------------------------
// Ввод: отслеживаем стрелки, F6 переключает режим
// ВАЖНО: в некоторых версиях Geode сигнатура без параметра timestamp.
// Если не компилируется — удали `double timestamp` (и в вызове оригинала).
// ---------------------------------------------------------------
class $modify(CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool isKeyRepeat, double timestamp) {
		switch (key) {
			case KEY_Up:    st::up    = down; break;
			case KEY_Down:  st::down  = down; break;
			case KEY_Left:  st::left  = down; break;
			case KEY_Right: st::right = down; break;
			case KEY_F6:
				if (down && !isKeyRepeat) {
					st::arrowMode = !st::arrowMode;
					log::info("Arrow mode: {}", st::arrowMode);
				}
				break;
			default: break;
		}
		return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, isKeyRepeat, timestamp);
	}
};

class $modify(PlayLayer) {
	void resetLevel() {
		PlayLayer::resetLevel();
		st::arrowMode = Mod::get()->getSettingValue<bool>("enabled-on-start");
	}
};

// ---------------------------------------------------------------
// НОВЫЙ РЕЖИМ: «Стрелочный полёт»
//   ↑  — подъём, отпустил — плавное падение
//   ←/→ — движение по горизонтали
// Коллизии со стенами/шипами обрабатывает сама игра.
// ---------------------------------------------------------------
class $modify(ArrowPlayer, PlayerObject) {
	void update(float dt) {
		PlayerObject::update(dt);

		if (!st::arrowMode) return;

		auto pl = PlayLayer::get();
		if (!pl) return;
		if (this != pl->m_player1 && this != pl->m_player2) return;
		if (this->m_isDead) return;

		auto mod = Mod::get();
		double climb = mod->getSettingValue<double>("climb-speed");
		double fall  = mod->getSettingValue<double>("fall-speed");
		double side  = mod->getSettingValue<double>("side-speed");

		// Вертикаль: плавно тянем скорость к целевой
		double target = st::up ? climb : -fall;
		if (st::down) target = -climb;
		this->m_yVelocity += (target - this->m_yVelocity) * std::min(1.0, dt * 12.0);

		// Горизонталь
		float dir = 0.f;
		if (st::left)  dir -= 1.f;
		if (st::right) dir += 1.f;
		if (dir != 0.f) {
			this->setPositionX(this->getPositionX() + dir * static_cast<float>(side) * dt);
		}
	}
};

// ---------------------------------------------------------------
// КАРКАС НОВЫХ ОРБОВ И ПАДОВ
// Реестр: ID объекта -> эффект. Эффекты уже написаны.
// Чтобы появились НОВЫЕ объекты в редакторе, нужно отдельно:
//   1) нарисовать спрайты и добавить в Resources мода,
//   2) зарегистрировать ID/иконки в редакторе (ObjectToolbox / EditorUI),
//   3) вписать эти ID в таблицы ниже.
// Пока ID — заглушки, поэтому в ванильных уровнях ничего не меняется.
// ---------------------------------------------------------------
using Effect = std::function<void(PlayerObject*)>;

namespace fx {
	constexpr int ORB_MEGA     = 90001; // орб: сильный толчок вверх
	constexpr int ORB_REVERSE  = 90002; // орб: разворот скорости по Y
	constexpr int ORB_TELEPORT = 90003; // орб: рывок вперёд
	constexpr int PAD_SUPER    = 90101; // пад: супер-прыжок
	constexpr int PAD_STOP     = 90102; // пад: гасит вертикальную скорость

	inline const std::unordered_map<int, Effect>& orbs() {
		static const std::unordered_map<int, Effect> m = {
			{ORB_MEGA,     [](PlayerObject* p) { p->m_yVelocity = 22.0 * (p->m_isUpsideDown ? -1 : 1); }},
			{ORB_REVERSE,  [](PlayerObject* p) { p->m_yVelocity = -p->m_yVelocity; }},
			{ORB_TELEPORT, [](PlayerObject* p) { p->setPositionX(p->getPositionX() + 120.f); }},
		};
		return m;
	}

	inline const std::unordered_map<int, Effect>& pads() {
		static const std::unordered_map<int, Effect> m = {
			{PAD_SUPER, [](PlayerObject* p) { p->m_yVelocity = 28.0 * (p->m_isUpsideDown ? -1 : 1); }},
			{PAD_STOP,  [](PlayerObject* p) { p->m_yVelocity = 0.0; }},
		};
		return m;
	}
}

#include <Geode/modify/GJBaseGameLayer.hpp>

// Орбы: применяем эффект после ванильной обработки
class $modify(OrbHook, PlayerObject) {
	void ringJump(RingObject* ring, bool p1) {
		PlayerObject::ringJump(ring, p1);
		if (!ring) return;
		auto it = fx::orbs().find(ring->m_objectID);
		if (it != fx::orbs().end()) it->second(this);
	}

	// Пады
	void bumpPlayer(float power, int type, bool noEffects, GameObject* obj) {
		PlayerObject::bumpPlayer(power, type, noEffects, obj);
		if (!obj) return;
		auto it = fx::pads().find(obj->m_objectID);
		if (it != fx::pads().end()) it->second(this);
	}
};
