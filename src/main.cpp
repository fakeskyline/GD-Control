#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

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
        FLAlertLayer::create(
            "GD CONTROL",
            "GD Control is loaded. Gameplay hooks are temporarily disabled while the Android 2.2091 compatibility issue is fixed.",
            "OK"
        )->show();
    }
};

$on_mod(Loaded) {
    log::info("GD Control v0.1.0 loaded.");
}
