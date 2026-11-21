#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>

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

// No-Clip: intercept the PlayLayer death path, not the inline
// GJBaseGameLayer::destroyPlayer implementation.
class $modify(GDControlPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) override {
        if (!isNoclipEnabled() || !player) {
            return PlayLayer::destroyPlayer(player, object);
        }

        // Keep RobTop's anticheat spike and level-completion paths intact.
        if ((m_anticheatSpike && object == m_anticheatSpike) ||
            m_levelEndAnimationStarted ||
            m_hasCompletedLevel) {
            return PlayLayer::destroyPlayer(player, object);
        }

        // Eclipse/GDH-style noclip: suppress the normal death call for
        // either active player. The original method is called when disabled.
        if (player == m_player1 || player == m_player2) {
            return;
        }

        PlayLayer::destroyPlayer(player, object);
    }
};

// Speedhack: scale the frame delta at the Cocos scheduler boundary.
// This is the established implementation used by GDH and Eclipse.
class $modify(GDControlScheduler, cocos2d::CCScheduler) {
    void update(float dt) override {
        auto speed = getSpeedMultiplier();
        if (speed > 0.f && speed != 1.f) {
            dt *= speed;
        }

        CCScheduler::update(dt);
    }
};

$on_mod(Loaded) {
    log::info("GD Control v0.1.4 loaded for Geometry Dash 2.209.");
}
