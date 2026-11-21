#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace {
    bool isNoclipEnabled() {
        return Mod::get()->getSettingValue<bool>("noclip-enabled");
    }

    float getSpeedMultiplier() {
        return std::clamp(
            Mod::get()->getSettingValue<float>("speed-multiplier"),
            0.25f,
            3.0f
        );
    }
}

class $modify(GDControlMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto buttonSprite = ButtonSprite::create(
            "GD CONTROL",
            "goldFont.fnt",
            "GJ_button_01.png",
            0.8f
        );

        if (buttonSprite) {
            auto button = CCMenuItemSpriteExtra::create(
                buttonSprite,
                this,
                menu_selector(GDControlMenuLayer::onGDControl)
            );

            if (button) {
                auto menu = CCMenu::create();
                menu->setPosition({0.f, 0.f});
                button->setPosition({winSize.width - 62.f, 42.f});
                menu->addChild(button);
                menu->setID("gd-control-menu"_spr);
                this->addChild(menu, 100);
            }
        }

        return true;
    }

    void onGDControl(CCObject*) {
        auto noclip = isNoclipEnabled() ? "ON" : "OFF";
        auto speed = getSpeedMultiplier();

        auto message = fmt::format(
            "NO-CLIP: {}\nSPEEDHACK: {:.2f}x\n\nChange options in Geode -> Mods -> GD Control -> Settings.",
            noclip,
            speed
        );

        FLAlertLayer::create("GD CONTROL", message, "OK")->show();
    }
};

// PlayLayer overrides destroyPlayer. Do not hook GJBaseGameLayer::destroyPlayer:
// it is inline in the 2.2091 bindings and Geode rejects that hook.
class $modify(GDControlPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        const bool mustKeepOriginalDeath =
            !isNoclipEnabled() ||
            !player ||
            object == m_anticheatSpike ||
            m_levelEndAnimationStarted;

        if (mustKeepOriginalDeath) {
            return PlayLayer::destroyPlayer(player, object);
        }

        // No-Clip: suppress the normal death path.
    }
};

// Scale only the gameplay layer delta, leaving the global Cocos scheduler
// and unrelated menu/UI animations at their normal speed.
class $modify(GDControlBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        if (typeinfo_cast<PlayLayer*>(this)) {
            dt *= getSpeedMultiplier();
        }

        GJBaseGameLayer::update(dt);
    }
};

$on_mod(Loaded) {
    log::info("GD Control v0.1.3 loaded.");
}
