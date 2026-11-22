#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/fmod/fmod.hpp>

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

    bool isPracticeMusicEnabled() {
        return Mod::get()->getSettingValue<bool>("unlock-practice-music");
    }

    bool isUnlockIconsEnabled() {
        return Mod::get()->getSettingValue<bool>("unlock-icons");
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
    CCLabelBMFont* m_iconsLabel = nullptr;
    CCLabelBMFont* m_practiceMusicLabel = nullptr;

    bool init() {
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto panelWidth = std::max(300.f, win.width - 24.f);
        auto panelHeight = std::max(220.f, win.height - 24.f);
        if (!Popup::init(panelWidth, panelHeight, "GJ_square01.png")) {
            return false;
        }

        this->setTitle("GD CONTROL", "goldFont.fnt", 0.85f);
        auto size = m_mainLayer->getContentSize();
        auto centerX = size.width * 0.5f;
        auto centerY = size.height * 0.5f;
        auto columnOffset = std::min(size.width * 0.24f, 190.f);
        auto leftX = centerX - columnOffset;
        auto rightX = centerX + columnOffset;

        auto makeButton = [this](const char* text, CCPoint pos, SEL_MenuHandler callback, float scale = 0.65f) {
            auto sprite = ButtonSprite::create(text, "goldFont.fnt", "GJ_button_01.png", scale);
            auto item = CCMenuItemSpriteExtra::create(sprite, this, callback);
            item->setPosition(pos);
            m_buttonMenu->addChild(item);
            return item;
        };

        auto addSectionTitle = [this](const char* text, CCPoint pos) {
            auto label = CCLabelBMFont::create(text, "bigFont.fnt");
            label->setScale(0.58f);
            label->setPosition(pos);
            m_mainLayer->addChild(label);
            return label;
        };

        auto rowTop = centerY + 48.f;
        auto rowBottom = centerY - 42.f;

        addSectionTitle("NO-CLIP", {leftX, rowTop});
        m_noclipLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_noclipLabel->setScale(0.72f);
        m_noclipLabel->setPosition({leftX, rowTop - 32.f});
        m_mainLayer->addChild(m_noclipLabel);
        makeButton("TOGGLE", {leftX, rowTop - 67.f},
            menu_selector(GDControlPopup::onToggleNoclip), 0.62f);

        addSectionTitle("SPEEDHACK", {rightX, rowTop});
        makeButton("-", {rightX - 72.f, rowTop - 40.f},
            menu_selector(GDControlPopup::onSpeedDown), 0.62f);
        m_speedLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_speedLabel->setScale(0.75f);
        m_speedLabel->setPosition({rightX, rowTop - 40.f});
        m_mainLayer->addChild(m_speedLabel);
        makeButton("+", {rightX + 72.f, rowTop - 40.f},
            menu_selector(GDControlPopup::onSpeedUp), 0.62f);

        addSectionTitle("UNLOCK ICONS", {leftX, rowBottom});
        m_iconsLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_iconsLabel->setScale(0.68f);
        m_iconsLabel->setPosition({leftX, rowBottom - 30.f});
        m_mainLayer->addChild(m_iconsLabel);
        makeButton("TOGGLE", {leftX, rowBottom - 65.f},
            menu_selector(GDControlPopup::onToggleIcons), 0.62f);

        addSectionTitle("PRACTICE MUSIC", {rightX, rowBottom});
        m_practiceMusicLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_practiceMusicLabel->setScale(0.68f);
        m_practiceMusicLabel->setPosition({rightX, rowBottom - 30.f});
        m_mainLayer->addChild(m_practiceMusicLabel);
        makeButton("UNLOCK", {rightX, rowBottom - 65.f},
            menu_selector(GDControlPopup::onTogglePracticeMusic), 0.62f);

        auto hint = CCLabelBMFont::create("LOCAL MOD SETTINGS", "goldFont.fnt");
        hint->setOpacity(170);
        hint->setScale(0.42f);
        hint->setPosition({centerX, 17.f});
        m_mainLayer->addChild(hint);

        this->refreshValues();
        return true;
    }

    void refreshValues() {
        if (m_noclipLabel) m_noclipLabel->setString(isNoclipEnabled() ? "ENABLED" : "DISABLED");
        if (m_speedLabel) m_speedLabel->setString(fmt::format("{:.2f}x", getSpeedMultiplier()).c_str());
        if (m_iconsLabel) m_iconsLabel->setString(isUnlockIconsEnabled() ? "ENABLED" : "DISABLED");
        if (m_practiceMusicLabel) m_practiceMusicLabel->setString(isPracticeMusicEnabled() ? "UNLOCKED" : "LOCKED");
    }

    void onToggleNoclip(CCObject*) {
        Mod::get()->setSettingValue<bool>("noclip-enabled", !isNoclipEnabled());
        this->refreshValues();
    }

    void onToggleIcons(CCObject*) {
        Mod::get()->setSettingValue<bool>("unlock-icons", !isUnlockIconsEnabled());
        this->refreshValues();
    }

    void onTogglePracticeMusic(CCObject*) {
        Mod::get()->setSettingValue<bool>("unlock-practice-music", !isPracticeMusicEnabled());
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

        // Scale the game clock and FMOD's master clock together.
        // The master channel group controls streamed level music too;
        // changing m_globalChannel alone does not reliably affect it.
        if (auto engine = FMODAudioEngine::sharedEngine()) {
            if (engine->m_system) {
                FMOD::ChannelGroup* masterGroup = nullptr;
                if (engine->m_system->getMasterChannelGroup(&masterGroup) == FMOD_OK && masterGroup) {
                    masterGroup->setPitch(speed);
                }
            }
        }

        CCScheduler::update(dt * speed);
    }
};

class $modify(GDControlStatsManager, GameStatsManager) {
    bool isItemUnlocked(UnlockType type, int itemID) {
        if (isPracticeMusicEnabled() && type == UnlockType::GJItem && itemID == 17) {
            return true;
        }
        return GameStatsManager::isItemUnlocked(type, itemID);
    }
};

class $modify(GDControlGameManager, GameManager) {
    bool isIconUnlocked(int iconID, IconType type) {
        if (isUnlockIconsEnabled()) {
            return true;
        }
        return GameManager::isIconUnlocked(iconID, type);
    }
};

$on_mod(Loaded) {
    log::info("GD Control v0.1.8 loaded for Geometry Dash 2.2091.");
}
