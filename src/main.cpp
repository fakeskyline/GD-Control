#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include <algorithm>
#include <cmath>

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

    void setSpeedMultiplier(float value) {
        value = std::round(std::clamp(value, 0.25f, 3.0f) * 20.f) / 20.f;
        Mod::get()->setSettingValue<float>("speed-multiplier", value);
    }
}

class GDControlPopup : public Popup {
protected:
    CCLabelBMFont* m_noclipLabel = nullptr;
    CCLabelBMFont* m_speedLabel = nullptr;

    bool init() {
        if (!Popup::init(310.f, 220.f, "GJ_square01.png")) {
            return false;
        }

        this->setTitle("GD CONTROL", "goldFont.fnt", 0.8f);

        auto size = m_mainLayer->getContentSize();

        auto makeButton = [this](const char* text, CCPoint pos, SEL_MenuHandler callback, float scale = 0.65f) {
            auto sprite = ButtonSprite::create(text, "goldFont.fnt", "GJ_button_01.png", scale);
            auto item = CCMenuItemSpriteExtra::create(sprite, this, callback);
            item->setPosition(pos);
            m_buttonMenu->addChild(item);
            return item;
        };

        auto noclipTitle = CCLabelBMFont::create("NO-CLIP", "bigFont.fnt");
        noclipTitle->setScale(0.55f);
        noclipTitle->setPosition({size.width / 2.f, 151.f});
        m_mainLayer->addChild(noclipTitle);

        m_noclipLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_noclipLabel->setScale(0.65f);
        m_noclipLabel->setPosition({size.width / 2.f, 126.f});
        m_mainLayer->addChild(m_noclipLabel);

        makeButton("TOGGLE", {size.width / 2.f, 96.f},
            menu_selector(GDControlPopup::onToggleNoclip), 0.55f);

        auto speedTitle = CCLabelBMFont::create("SPEEDHACK", "bigFont.fnt");
        speedTitle->setScale(0.55f);
        speedTitle->setPosition({size.width / 2.f, 62.f});
        m_mainLayer->addChild(speedTitle);

        makeButton("-", {size.width / 2.f - 75.f, 29.f},
            menu_selector(GDControlPopup::onSpeedDown), 0.55f);

        m_speedLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_speedLabel->setScale(0.7f);
        m_speedLabel->setPosition({size.width / 2.f, 29.f});
        m_mainLayer->addChild(m_speedLabel);

        makeButton("+", {size.width / 2.f + 75.f, 29.f},
            menu_selector(GDControlPopup::onSpeedUp), 0.55f);

        this->refreshValues();
        return true;
    }

    void refreshValues() {
        if (m_noclipLabel) {
            m_noclipLabel->setString(isNoclipEnabled() ? "ENABLED" : "DISABLED");
        }
        if (m_speedLabel) {
            m_speedLabel->setString(fmt::format("{:.2f}x", getSpeedMultiplier()).c_str());
        }
    }

    void onToggleNoclip(CCObject*) {
        Mod::get()->setSettingValue<bool>("noclip-enabled", !isNoclipEnabled());
        this->refreshValues();
    }

    void onSpeedDown(CCObject*) {
        setSpeedMultiplier(getSpeedMultiplier() - 0.05f);
        this->refreshValues();
    }

    void onSpeedUp(CCObject*) {
        setSpeedMultiplier(getSpeedMultiplier() + 0.05f);
        this->refreshValues();
    }

public:
    static GDControlPopup* create() {
        auto ret = new GDControlPopup();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class GDFloatingButton : public CCNode, public cocos2d::CCTargetedTouchDelegate {
protected:
    bool m_dragging = false;
    bool m_moved = false;
    CCPoint m_touchOffset = CCPointZero;
    CCPoint m_touchStart = CCPointZero;
    CCPoint m_startPosition = CCPointZero;

    bool init() {
        if (!CCNode::init()) {
            return false;
        }

        auto sprite = ButtonSprite::create(
            "GD CONTROL",
            "goldFont.fnt",
            "GJ_button_01.png",
            0.65f
        );
        if (!sprite) {
            return false;
        }

        this->setContentSize(sprite->getContentSize());
        this->setAnchorPoint({0.5f, 0.5f});
        sprite->setPosition(this->getContentSize() / 2.f);
        this->addChild(sprite);

        return true;
    }

    void onEnter() override {
        CCNode::onEnter();
        CCDirector::sharedDirector()->getTouchDispatcher()
            ->addTargetedDelegate(this, -500, true);
    }

    void onExit() override {
        CCDirector::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
        CCNode::onExit();
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (!this->isVisible() || !this->isRunning()) {
            return false;
        }

        auto local = this->convertToNodeSpace(touch->getLocation());
        CCRect bounds(0.f, 0.f, this->getContentSize().width, this->getContentSize().height);
        if (!bounds.containsPoint(local)) {
            return false;
        }

        m_dragging = true;
        m_moved = false;
        m_touchStart = touch->getLocation();
        m_startPosition = this->getPosition();
        m_touchOffset = m_startPosition - m_touchStart;
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!m_dragging) {
            return;
        }

        auto current = touch->getLocation();
        auto delta = current - m_touchStart;
        if (std::fabs(delta.x) + std::fabs(delta.y) > 8.f) {
            m_moved = true;
        }

        if (m_moved) {
            auto parent = this->getParent();
            if (!parent) {
                return;
            }

            auto target = parent->convertToNodeSpace(current + m_touchOffset);
            auto win = CCDirector::sharedDirector()->getWinSize();
            auto halfW = this->getContentSize().width * 0.5f;
            auto halfH = this->getContentSize().height * 0.5f;

            target.x = std::clamp(target.x, halfW, win.width - halfW);
            target.y = std::clamp(target.y, halfH, win.height - halfH);
            this->setPosition(target);
        }
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (!m_dragging) {
            return;
        }

        m_dragging = false;

        if (m_moved) {
            Mod::get()->setSavedValue<float>("floating-button-x", this->getPositionX());
            Mod::get()->setSavedValue<float>("floating-button-y", this->getPositionY());
        }
        else {
            if (auto popup = GDControlPopup::create()) {
                popup->show();
            }
        }
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        m_dragging = false;
    }

public:
    static GDFloatingButton* create() {
        auto ret = new GDFloatingButton();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(GDControlMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        auto button = GDFloatingButton::create();
        if (button) {
            auto win = CCDirector::sharedDirector()->getWinSize();
            auto x = Mod::get()->getSavedValue<float>("floating-button-x", win.width - 76.f);
            auto y = Mod::get()->getSavedValue<float>("floating-button-y", 48.f);

            button->setPosition({
                std::clamp(x, button->getContentSize().width / 2.f, win.width - button->getContentSize().width / 2.f),
                std::clamp(y, button->getContentSize().height / 2.f, win.height - button->getContentSize().height / 2.f)
            });

            button->setID("gd-control-floating-button"_spr);
            this->addChild(button, 1000);
        }

        return true;
    }
};

class $modify(GDControlPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) override {
        if (!isNoclipEnabled() || !player) {
            return PlayLayer::destroyPlayer(player, object);
        }

        if ((m_anticheatSpike && object == m_anticheatSpike) ||
            m_levelEndAnimationStarted ||
            m_hasCompletedLevel) {
            return PlayLayer::destroyPlayer(player, object);
        }

        if (player == m_player1 || player == m_player2) {
            return;
        }

        PlayLayer::destroyPlayer(player, object);
    }
};

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
    log::info("GD Control v0.1.6 loaded for Geometry Dash 2.2091.");
}
