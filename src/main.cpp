#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace {
    bool isNoclipEnabled() {
        return Mod::get()->getSettingValue<bool>("noclip-enabled");
    }

    float getSpeedMultiplier() {
        auto value = Mod::get()->getSettingValue<float>("speed-multiplier");
        return std::clamp(value, 0.25f, 3.0f);
    }
}

class $modify(GDControlMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto buttonSprite = ButtonSprite::create(
            "CONTROL",
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
                button->setPosition({winSize.width - 48.f, 42.f});
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
            "NO-CLIP: {}\nSPEED: {:.2f}x\n\nOpen Geode -> Mods -> GD Control -> Settings to change these options.",
            noclip,
            speed
        );

        FLAlertLayer::create("GD CONTROL", message, "OK")->show();
    }
};

class $modify(GDControlPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* cause) {
        if (isNoclipEnabled()) {
            return;
        }

        PlayLayer::destroyPlayer(player, cause);
    }

    void update(float dt) {
        PlayLayer::update(dt * getSpeedMultiplier());
    }
};

$on_mod(Loaded) {
    log::info("GD Control v0.1.0 loaded.");
}
