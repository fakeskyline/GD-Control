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
    CCLabelTTF* m_noclipLabel = nullptr;
    CCLabelTTF* m_speedLabel = nullptr;
    CCLabelTTF* m_iconsLabel = nullptr;
    CCLabelTTF* m_practiceMusicLabel = nullptr;
    CCLabelTTF* m_titleLabel = nullptr;
    CCLabelTTF* m_languageLabel = nullptr;
    CCLabelTTF* m_noclipTitle = nullptr;
    CCLabelTTF* m_speedTitle = nullptr;
    CCLabelTTF* m_iconsTitle = nullptr;
    CCLabelTTF* m_musicTitle = nullptr;
    CCLabelTTF* m_noclipButtonLabel = nullptr;
    CCLabelTTF* m_iconsButtonLabel = nullptr;
    CCLabelTTF* m_musicButtonLabel = nullptr;
    CCLabelTTF* m_speedButtonLabel = nullptr;
    CCLabelTTF* m_hintLabel = nullptr;

    bool isRussian() {
        return Mod::get()->getSavedValue<bool>("ui-russian", false);
    }

    const char* tr(const char* en, const char* ru) {
        return isRussian() ? ru : en;
    }

    CCLabelTTF* addText(const char* text, CCPoint pos, float size, ccColor3B color,
                        CCNode* parent = nullptr) {
        auto label = CCLabelTTF::create(text, "Arial", size);
        label->setColor(color);
        label->setAnchorPoint({0.5f, 0.5f});
        label->setPosition(pos);
        (parent ? parent : static_cast<CCNode*>(m_mainLayer))->addChild(label);
        return label;
    }

    CCNode* addCard(float x, float y, float w, float h) {
        auto card = CCLayerColor::create(ccc4(39, 45, 58, 255), w, h);
        card->setPosition({x - w * 0.5f, y - h * 0.5f});
        m_mainLayer->addChild(card, 1);
        return card;
    }

    CCMenuItemSpriteExtra* addActionButton(const char* text, CCPoint pos, float w,
                                           SEL_MenuHandler callback) {
        auto bg = CCLayerColor::create(ccc4(65, 105, 220, 255), w, 42.f);
        auto label = CCLabelTTF::create(text, "Arial", 17.f);
        label->setColor(ccc3(255, 255, 255));
        label->setPosition({w * 0.5f, 21.f});
        bg->addChild(label);
        auto item = CCMenuItemSpriteExtra::create(bg, this, callback);
        item->setPosition(pos);
        m_buttonMenu->addChild(item);
        return item;
    }

    CCMenuItemSpriteExtra* addSmallButton(const char* text, CCPoint pos,
                                          SEL_MenuHandler callback) {
        auto bg = CCLayerColor::create(ccc4(65, 105, 220, 255), 42.f, 38.f);
        auto label = CCLabelTTF::create(text, "Arial", 20.f);
        label->setColor(ccc3(255, 255, 255));
        label->setPosition({21.f, 19.f});
        bg->addChild(label);
        auto item = CCMenuItemSpriteExtra::create(bg, this, callback);
        item->setPosition(pos);
        m_buttonMenu->addChild(item);
        return item;
    }

    void buildCard(float centerX, float centerY, float cardW, float cardH,
                   const char* title, CCLabelTTF*& titleLabel,
                   CCLabelTTF*& statusLabel, CCLabelTTF*& buttonLabel,
                   const char* status, SEL_MenuHandler callback,
                   bool speedCard = false) {
        auto card = addCard(centerX, centerY, cardW, cardH);
        titleLabel = addText(title, {cardW * 0.5f, cardH * 0.76f}, 19.f,
                             ccc3(224, 229, 240), card);
        if (speedCard) {
            m_speedLabel = addText(status, {cardW * 0.5f, cardH * 0.39f}, 23.f,
                                   ccc3(190, 205, 255), card);
            addSmallButton("-", {centerX - cardW * 0.27f, centerY - cardH * 0.11f},
                           menu_selector(GDControlPopup::onSpeedDown));
            addSmallButton("+", {centerX + cardW * 0.27f, centerY - cardH * 0.11f},
                           menu_selector(GDControlPopup::onSpeedUp));
        } else {
            statusLabel = addText(status, {cardW * 0.5f, cardH * 0.48f}, 15.f,
                                  ccc3(174, 190, 214), card);
            buttonLabel = addText("", {cardW * 0.5f, 21.f}, 1.f,
                                  ccc3(255, 255, 255), card);
            buttonLabel->setVisible(false);
            auto btn = CCLayerColor::create(ccc4(65, 105, 220, 255), std::min(150.f, cardW * 0.62f), 40.f);
            auto btnText = CCLabelTTF::create(tr("Change", "Изменить"), "Arial", 16.f);
            btnText->setColor(ccc3(255, 255, 255));
            btnText->setPosition({btn->getContentSize().width * 0.5f, 20.f});
            btn->addChild(btnText);
            auto item = CCMenuItemSpriteExtra::create(btn, this, callback);
            item->setPosition({centerX, centerY - cardH * 0.22f});
            m_buttonMenu->addChild(item);
            buttonLabel = btnText;
        }
    }

    bool init() {
        auto win = CCDirector::sharedDirector()->getWinSize();
        const float panelW = win.width * 0.94f;
        const float panelH = win.height * 0.92f;
        if (!Popup::init(panelW, panelH, "GJ_square01.png")) return false;

        auto size = m_mainLayer->getContentSize();
        const float w = size.width;
        const float h = size.height;

        auto surface = CCLayerColor::create(ccc4(25, 29, 38, 255), w, h);
        surface->setPosition({0.f, 0.f});
        m_mainLayer->addChild(surface, -10);

        m_titleLabel = addText("GD CONTROL", {w * 0.5f, h * 0.91f}, 25.f,
                               ccc3(238, 241, 248));
        m_languageLabel = addText("", {w * 0.84f, h * 0.91f}, 13.f,
                                  ccc3(190, 205, 255));
        auto langBg = CCLayerColor::create(ccc4(49, 58, 76, 255), 74.f, 32.f);
        auto langText = CCLabelTTF::create("", "Arial", 13.f);
        langText->setColor(ccc3(220, 228, 245));
        langText->setPosition({37.f, 16.f});
        langBg->addChild(langText);
        m_languageLabel = langText;
        auto langItem = CCMenuItemSpriteExtra::create(langBg, this, menu_selector(GDControlPopup::onToggleLanguage));
        langItem->setPosition({w * 0.84f, h * 0.91f});
        m_buttonMenu->addChild(langItem);

        const bool landscape = w >= h * 1.15f;
        const float gap = landscape ? w * 0.035f : w * 0.04f;
        const float cardW = landscape ? (w - gap * 3.f) * 0.5f : w * 0.86f;
        const float cardH = landscape ? h * 0.29f : h * 0.165f;
        const float top = h * 0.69f;
        const float bottom = landscape ? h * 0.34f : h * 0.17f;
        const float left = landscape ? w * 0.25f : w * 0.5f;
        const float right = landscape ? w * 0.75f : w * 0.5f;

        buildCard(left, top, cardW, cardH, tr("NO-CLIP", "НОУКЛИП"),
                  m_noclipTitle, m_noclipLabel, m_noclipButtonLabel,
                  isNoclipEnabled() ? "ON" : "OFF",
                  menu_selector(GDControlPopup::onToggleNoclip));
        buildCard(right, top, cardW, cardH, tr("SPEEDHACK", "УСКОРЕНИЕ"),
                  m_speedTitle, m_speedLabel, m_speedButtonLabel,
                  fmt::format("{:.2f}x", getSpeedMultiplier()).c_str(),
                  nullptr, true);
        buildCard(left, bottom, cardW, cardH, tr("UNLOCK ICONS", "РАЗБЛОКИРОВКА ИКОНОК"),
                  m_iconsTitle, m_iconsLabel, m_iconsButtonLabel,
                  isUnlockIconsEnabled() ? "ON" : "OFF",
                  menu_selector(GDControlPopup::onToggleIcons));
        buildCard(right, bottom, cardW, cardH, tr("PRACTICE MUSIC", "МУЗЫКА В ПРАКТИКЕ"),
                  m_musicTitle, m_practiceMusicLabel, m_musicButtonLabel,
                  isPracticeMusicEnabled() ? "ON" : "OFF",
                  menu_selector(GDControlPopup::onTogglePracticeMusic));

        m_hintLabel = addText(tr("Local settings • changes apply instantly",
                                 "Локальные настройки • изменения сразу применяются"),
                              {w * 0.5f, h * 0.045f}, 12.f,
                              ccc3(137, 149, 171));
        refreshValues();
        return true;
    }

    void refreshValues() {
        if (m_noclipLabel) m_noclipLabel->setString(isNoclipEnabled() ? "ON" : "OFF");
        if (m_speedLabel) m_speedLabel->setString(fmt::format("{:.2f}x", getSpeedMultiplier()).c_str());
        if (m_iconsLabel) m_iconsLabel->setString(isUnlockIconsEnabled() ? "ON" : "OFF");
        if (m_practiceMusicLabel) m_practiceMusicLabel->setString(isPracticeMusicEnabled() ? "ON" : "OFF");

        if (m_titleLabel) m_titleLabel->setString("GD CONTROL");
        if (m_languageLabel) m_languageLabel->setString(isRussian() ? "RU" : "EN");
        if (m_noclipTitle) m_noclipTitle->setString(tr("NO-CLIP", "НОУКЛИП"));
        if (m_speedTitle) m_speedTitle->setString(tr("SPEEDHACK", "УСКОРЕНИЕ"));
        if (m_iconsTitle) m_iconsTitle->setString(tr("UNLOCK ICONS", "РАЗБЛОКИРОВКА ИКОНОК"));
        if (m_musicTitle) m_musicTitle->setString(tr("PRACTICE MUSIC", "МУЗЫКА В ПРАКТИКЕ"));
        if (m_noclipButtonLabel) m_noclipButtonLabel->setString(tr("Change", "Изменить"));
        if (m_iconsButtonLabel) m_iconsButtonLabel->setString(tr("Change", "Изменить"));
        if (m_musicButtonLabel) m_musicButtonLabel->setString(tr("Change", "Изменить"));
        if (m_hintLabel) m_hintLabel->setString(tr("Local settings • changes apply instantly",
                                                   "Локальные настройки • изменения сразу применяются"));
    }

    void onToggleLanguage(CCObject*) {
        Mod::get()->setSavedValue<bool>("ui-russian", !isRussian());
        refreshValues();
    }

    void onToggleNoclip(CCObject*) {
        Mod::get()->setSettingValue<bool>("noclip-enabled", !isNoclipEnabled());
        refreshValues();
    }
    void onToggleIcons(CCObject*) {
        Mod::get()->setSettingValue<bool>("unlock-icons", !isUnlockIconsEnabled());
        refreshValues();
    }
    void onTogglePracticeMusic(CCObject*) {
        Mod::get()->setSettingValue<bool>("unlock-practice-music", !isPracticeMusicEnabled());
        refreshValues();
    }
    void onSpeedDown(CCObject*) {
        setSpeedMultiplier(getSpeedMultiplier() - 0.05f);
        refreshValues();
    }
    void onSpeedUp(CCObject*) {
        setSpeedMultiplier(getSpeedMultiplier() + 0.05f);
        refreshValues();
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
            "GDC",
            "bigFont.fnt",
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
