#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/Button.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/fmod/fmod.hpp>

#include <algorithm>
#include <cmath>
#include <functional>

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
    CCLabelBMFont* m_languageLabel = nullptr;
    CCLabelBMFont* m_noclipTitle = nullptr;
    CCLabelBMFont* m_speedTitle = nullptr;
    CCLabelBMFont* m_iconsTitle = nullptr;
    CCLabelBMFont* m_musicTitle = nullptr;
    CCLabelBMFont* m_noclipAction = nullptr;
    CCLabelBMFont* m_iconsAction = nullptr;
    CCLabelBMFont* m_musicAction = nullptr;
    CCLabelBMFont* m_hintLabel = nullptr;
    float m_cardTitleMaxWidth = 0.f;

    bool isRussian() {
        return Mod::get()->getSavedValue<bool>("ui-russian", false);
    }

    const char* tr(const char* en, const char* ru) {
        return isRussian() ? ru : en;
    }

    CCLabelBMFont* addText(CCNode* parent, const char* text, CCPoint pos,
                        float fontSize, ccColor3B color, float maxWidth = 0.f) {
        auto label = CCLabelBMFont::create(text, "GoogleSans.fnt"_spr);
        const float baseScale = fontSize / 80.f;
        label->setScale(baseScale);
        label->setColor(color);
        label->setAnchorPoint({0.5f, 0.5f});
        if (maxWidth > 0.f && label->getContentSize().width * baseScale > maxWidth) {
            label->setScale(baseScale * maxWidth / label->getContentSize().width);
        }
        label->setPosition(pos);
        parent->addChild(label);
        return label;
    }

    geode::NineSlice* makeSurface(CCSize size, ccColor3B color) {
        auto surface = geode::NineSlice::create(
            "md3_round.png"_spr, {}, {24.f, 24.f, 24.f, 24.f}
        );
        surface->setContentSize(size);
        surface->setColor(color);
        surface->setOpacity(255);
        return surface;
    }

    geode::Button* makeButton(CCNode* parent, const char* text, CCPoint pos,
                                  CCSize size, std::function<void()> callback,
                                  bool primary = false, CCLabelBMFont** outLabel = nullptr) {
        auto bg = makeSurface(size, primary ? ccc3(75, 94, 166) : ccc3(58, 62, 76));
        auto label = CCLabelBMFont::create(text, "GoogleSans.fnt"_spr);
        label->setScale(15.f / 80.f);
        label->setColor(primary ? ccc3(30, 39, 64) : ccc3(220, 222, 235));
        label->setPosition({size.width * 0.5f, size.height * 0.5f});
        bg->addChild(label);
        if (outLabel) *outLabel = label;

        auto button = geode::Button::createWithNode(bg, [callback](geode::Button*) {
            callback();
        });
        button->setPosition(pos);
        button->setAnimationType(geode::Button::AnimationType::Scale);
        button->setScaleMultiplier(0.97f);
        parent->addChild(button, 5);
        return button;
    }

    CCNode* createCard(float centerX, float centerY, float cardW, float cardH) {
        auto card = CCNode::create();
        card->setContentSize({cardW, cardH});
        card->setAnchorPoint({0.5f, 0.5f});
        card->setPosition({centerX, centerY});
        auto bg = makeSurface({cardW, cardH}, ccc3(42, 46, 58));
        bg->setPosition({cardW * 0.5f, cardH * 0.5f});
        card->addChild(bg);
        m_mainLayer->addChild(card, 2);
        return card;
    }

    void addToggleCard(float centerX, float centerY, float cardW, float cardH,
                       const char* title, const char* status, bool enabled,
                       CCLabelBMFont*& titleLabel, CCLabelBMFont*& statusLabel,
                       CCLabelBMFont*& actionLabel, std::function<void()> callback) {
        auto card = createCard(centerX, centerY, cardW, cardH);
        titleLabel = addText(card, title, {cardW * 0.5f, cardH * 0.76f},
                             18.f, ccc3(229, 226, 238), cardW * 0.88f);
        statusLabel = addText(card, status, {cardW * 0.5f, cardH * 0.49f},
                              14.f, enabled ? ccc3(190, 210, 255) : ccc3(177, 180, 194));
        auto button = makeButton(card, tr("Toggle", "Переключить"),
                                 {cardW * 0.5f, cardH * 0.20f},
                                 {std::min(154.f, cardW * 0.56f), 38.f},
                                 callback, true, &actionLabel);
    }

    void addSpeedCard(float centerX, float centerY, float cardW, float cardH) {
        auto card = createCard(centerX, centerY, cardW, cardH);
        m_speedTitle = addText(card, tr("SPEEDHACK", "УСКОРЕНИЕ"),
                               {cardW * 0.5f, cardH * 0.76f}, 18.f,
                               ccc3(229, 226, 238), cardW * 0.88f);
        m_speedLabel = addText(card, "", {cardW * 0.5f, cardH * 0.40f},
                               23.f, ccc3(208, 188, 255));
        makeButton(card, "−", {cardW * 0.25f, cardH * 0.40f},
                   {44.f, 38.f}, [this] { onSpeedDown(nullptr); });
        makeButton(card, "+", {cardW * 0.75f, cardH * 0.40f},
                   {44.f, 38.f}, [this] { onSpeedUp(nullptr); });
    }

    bool init() {
        auto win = CCDirector::sharedDirector()->getWinSize();
        const float panelW = win.width * 0.94f;
        const float panelH = win.height * 0.92f;
        if (!Popup::init(panelW, panelH, "GJ_square01.png")) return false;

        auto size = m_mainLayer->getContentSize();
        const float w = size.width;
        const float h = size.height;

        auto surface = makeSurface({w, h}, ccc3(27, 28, 34));
        surface->setPosition({w * 0.5f, h * 0.5f});
        m_mainLayer->addChild(surface, 0);

        addText(m_mainLayer, "GD CONTROL", {w * 0.5f, h * 0.91f},
                25.f, ccc3(242, 240, 248));
        auto langBg = makeSurface({66.f, 32.f}, ccc3(54, 58, 72));
        auto langText = CCLabelBMFont::create("EN", "GoogleSans.fnt"_spr);
        langText->setScale(13.f / 80.f);
        langText->setColor(ccc3(224, 222, 236));
        langText->setPosition({33.f, 16.f});
        langBg->addChild(langText);
        auto langButton = geode::Button::createWithNode(langBg, [this](geode::Button*) {
            onToggleLanguage(nullptr);
        });
        langButton->setPosition({w * 0.87f, h * 0.91f});
        m_mainLayer->addChild(langButton, 10);
        m_languageLabel = langText;

        const bool landscape = w >= h * 1.15f;
        const float gapX = landscape ? w * 0.035f : 0.f;
        const float cardW = landscape ? (w - gapX * 3.f) * 0.5f : w * 0.88f;
        const float cardH = landscape ? h * 0.30f : h * 0.17f;
        m_cardTitleMaxWidth = cardW * 0.88f;
        const float leftX = landscape ? w * 0.25f : w * 0.5f;
        const float rightX = landscape ? w * 0.75f : w * 0.5f;
        const float topY = h * 0.68f;
        const float bottomY = landscape ? h * 0.32f : h * 0.15f;

        addToggleCard(leftX, topY, cardW, cardH,
                      tr("NO-CLIP", "НОУКЛИП"),
                      isNoclipEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"),
                      isNoclipEnabled(), m_noclipTitle, m_noclipLabel, m_noclipAction,
                      [this] { onToggleNoclip(nullptr); });
        addSpeedCard(rightX, topY, cardW, cardH);
        addToggleCard(leftX, bottomY, cardW, cardH,
                      tr("UNLOCK ICONS", "РАЗБЛОКИРОВКА ИКОНОК"),
                      isUnlockIconsEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"),
                      isUnlockIconsEnabled(), m_iconsTitle, m_iconsLabel, m_iconsAction,
                      [this] { onToggleIcons(nullptr); });
        addToggleCard(rightX, bottomY, cardW, cardH,
                      tr("PRACTICE MUSIC", "МУЗЫКА В ПРАКТИКЕ"),
                      isPracticeMusicEnabled() ? tr("Unlocked", "Разблокировано") : tr("Locked", "Заблокировано"),
                      isPracticeMusicEnabled(), m_musicTitle, m_practiceMusicLabel, m_musicAction,
                      [this] { onTogglePracticeMusic(nullptr); });

        m_hintLabel = addText(m_mainLayer,
            tr("Local settings · changes apply instantly",
               "Локальные настройки · изменения применяются сразу"),
            {w * 0.5f, h * 0.045f}, 12.f, ccc3(160, 163, 177), w * 0.78f);
        refreshValues();
        return true;
    }

    void refreshValues() {
        if (m_noclipLabel) m_noclipLabel->setString(isNoclipEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"));
        if (m_speedLabel) m_speedLabel->setString(fmt::format("{:.2f}x", getSpeedMultiplier()).c_str());
        if (m_iconsLabel) m_iconsLabel->setString(isUnlockIconsEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"));
        if (m_practiceMusicLabel) m_practiceMusicLabel->setString(isPracticeMusicEnabled() ? tr("Unlocked", "Разблокировано") : tr("Locked", "Заблокировано"));
        if (m_noclipTitle) { m_noclipTitle->setString(tr("NO-CLIP", "НОУКЛИП")); m_noclipTitle->setScale(std::min(1.f, m_cardTitleMaxWidth / m_noclipTitle->getContentSize().width)); }
        if (m_speedTitle) { m_speedTitle->setString(tr("SPEEDHACK", "УСКОРЕНИЕ")); m_speedTitle->setScale(std::min(1.f, m_cardTitleMaxWidth / m_speedTitle->getContentSize().width)); }
        if (m_iconsTitle) { m_iconsTitle->setString(tr("UNLOCK ICONS", "РАЗБЛОКИРОВКА ИКОНОК")); m_iconsTitle->setScale(std::min(1.f, m_cardTitleMaxWidth / m_iconsTitle->getContentSize().width)); }
        if (m_musicTitle) { m_musicTitle->setString(tr("PRACTICE MUSIC", "МУЗЫКА В ПРАКТИКЕ")); m_musicTitle->setScale(std::min(1.f, m_cardTitleMaxWidth / m_musicTitle->getContentSize().width)); }
        if (m_languageLabel) m_languageLabel->setString(isRussian() ? "RU" : "EN");
        if (m_noclipAction) m_noclipAction->setString(tr("Toggle", "Переключить"));
        if (m_iconsAction) m_iconsAction->setString(tr("Toggle", "Переключить"));
        if (m_musicAction) m_musicAction->setString(tr("Toggle", "Переключить"));
        if (m_hintLabel) m_hintLabel->setString(tr("Local settings · changes apply instantly",
                                                   "Локальные настройки · изменения применяются сразу"));
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
