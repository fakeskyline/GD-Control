#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/Button.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/fmod/fmod.hpp>

#include <algorithm>
#include <cmath>
#include <chrono>
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

    bool isAutoClickerEnabled() {
        return Mod::get()->getSavedValue<bool>("autoclicker-enabled", false);
    }

    int getAutoClickerHold() {
        return static_cast<int>(std::clamp<int64_t>(
            Mod::get()->getSavedValue<int64_t>("autoclicker-hold", 1), 1, 20
        ));
    }

    int getAutoClickerRelease() {
        return static_cast<int>(std::clamp<int64_t>(
            Mod::get()->getSavedValue<int64_t>("autoclicker-release", 1), 1, 20
        ));
    }

    int getTPSBypassValue() {
        return static_cast<int>(std::clamp<int64_t>(
            Mod::get()->getSavedValue<int64_t>("tps-bypass-value", 240), 30, 2400
        ));
    }

    bool isTPSBypassEnabled() {
        return Mod::get()->getSavedValue<bool>("tps-bypass-enabled", false);
    }

    int getFPSBypassValue() {
        return static_cast<int>(std::clamp<int64_t>(
            Mod::get()->getSavedValue<int64_t>("fps-bypass-value", 60), 30, 1000
        ));
    }

    bool isFPSBypassEnabled() {
        return Mod::get()->getSavedValue<bool>("fps-bypass-enabled", false);
    }

    void applyFPSBypass() {
        if (isFPSBypassEnabled()) {
            CCDirector::sharedDirector()->setAnimationInterval(
                1.0 / static_cast<double>(getFPSBypassValue())
            );
        } else if (auto manager = GameManager::sharedState()) {
            manager->updateCustomFPS();
        }
    }

    bool g_autoClickerHeld = false;
    bool g_autoClickerInjected = false;
    bool g_autoClickerStarted = false;
    int g_autoClickerCounter = 0;

    void stopAutoClicker(GJBaseGameLayer* layer) {
        if (layer && g_autoClickerHeld) {
            g_autoClickerInjected = true;
            layer->handleButton(false, 1, true);
            g_autoClickerInjected = false;
        }
        g_autoClickerHeld = false;
        g_autoClickerStarted = false;
        g_autoClickerCounter = 0;
    }
}

class GDControlPopup : public Popup {
protected:
    CCLabelTTF* m_titleLabel = nullptr;
    CCLabelTTF* m_languageLabel = nullptr;
    CCLabelTTF* m_hintLabel = nullptr;

    CCLabelTTF* m_noclipTitle = nullptr;
    CCLabelTTF* m_noclipLabel = nullptr;
    CCLabelTTF* m_noclipAction = nullptr;
    CCLabelTTF* m_speedTitle = nullptr;
    CCLabelTTF* m_speedLabel = nullptr;
    CCLabelTTF* m_iconsTitle = nullptr;
    CCLabelTTF* m_iconsLabel = nullptr;
    CCLabelTTF* m_iconsAction = nullptr;
    CCLabelTTF* m_musicTitle = nullptr;
    CCLabelTTF* m_musicLabel = nullptr;
    CCLabelTTF* m_musicAction = nullptr;

    CCLabelTTF* m_autoTitle = nullptr;
    CCLabelTTF* m_autoStatus = nullptr;
    CCLabelTTF* m_autoAction = nullptr;
    CCLabelTTF* m_autoHold = nullptr;
    CCLabelTTF* m_autoRelease = nullptr;

    CCLabelTTF* m_tpsTitle = nullptr;
    CCLabelTTF* m_tpsStatus = nullptr;
    CCLabelTTF* m_tpsValue = nullptr;
    CCLabelTTF* m_tpsAction = nullptr;
    CCLabelTTF* m_fpsTitle = nullptr;
    CCLabelTTF* m_fpsStatus = nullptr;
    CCLabelTTF* m_fpsValue = nullptr;
    CCLabelTTF* m_fpsAction = nullptr;

    float m_cardTitleMaxWidth = 0.f;

    bool isRussian() {
        return Mod::get()->getSavedValue<bool>("ui-russian", false);
    }

    const char* tr(const char* en, const char* ru) {
        return isRussian() ? ru : en;
    }

    CCLabelTTF* addText(CCNode* parent, const char* text, CCPoint pos,
                        float size, ccColor3B color, float maxWidth = 0.f) {
        auto label = CCLabelTTF::create(text, "Arial", size);
        if (!label) return nullptr;
        label->setColor(color);
        label->setAnchorPoint({0.5f, 0.5f});
        if (maxWidth > 0.f && label->getContentSize().width > maxWidth) {
            label->setScale(maxWidth / label->getContentSize().width);
        }
        label->setPosition(pos);
        parent->addChild(label);
        return label;
    }

    CCNode* addCard(float centerX, float centerY, float cardW, float cardH) {
        auto card = geode::NineSlice::create("GJ_square01.png", {}, {12.f, 12.f, 12.f, 12.f});
        if (!card) return nullptr;
        card->setContentSize({cardW, cardH});
        card->setColor(ccc3(34, 78, 190));
        card->setPosition({centerX - cardW * 0.5f, centerY - cardH * 0.5f});
        m_mainLayer->addChild(card, 2);
        return card;
    }

    geode::Button* addActionButton(CCNode* parent, const char* text, CCPoint pos,
                                   float width, float height,
                                   std::function<void()> callback,
                                   CCLabelTTF** outLabel = nullptr) {
        auto bg = geode::NineSlice::create("GJ_button_01.png", {}, {9.f, 9.f, 9.f, 9.f});
        if (!bg) return nullptr;
        bg->setContentSize({width, height});

        auto label = CCLabelTTF::create(text, "Arial", std::min(13.f, height * 0.46f));
        if (!label) return nullptr;
        label->setColor(ccc3(255, 255, 255));
        label->setPosition({width * 0.5f, height * 0.5f});
        const float maxLabelWidth = width - 12.f;
        if (label->getContentSize().width > maxLabelWidth) {
            label->setScale(maxLabelWidth / label->getContentSize().width);
        }
        bg->addChild(label, 5);

        auto button = geode::Button::createWithNode(
            bg,
            [callback = std::move(callback)](geode::Button*) {
                if (callback) callback();
            }
        );
        if (!button) return nullptr;
        button->setPosition(pos);
        button->setAnimationType(geode::Button::AnimationType::Scale);
        parent->addChild(button, 20);
        if (outLabel) *outLabel = label;
        return button;
    }

    geode::Button* addSmallButton(CCNode* parent, const char* text, CCPoint pos,
                                  std::function<void()> callback) {
        auto bg = geode::NineSlice::create("GJ_button_02.png", {}, {8.f, 8.f, 8.f, 8.f});
        if (!bg) return nullptr;
        constexpr float buttonW = 30.f;
        constexpr float buttonH = 27.f;
        bg->setContentSize({buttonW, buttonH});

        auto label = CCLabelTTF::create(text, "Arial", 15.f);
        if (!label) return nullptr;
        label->setColor(ccc3(255, 255, 255));
        label->setPosition({buttonW * 0.5f, buttonH * 0.5f});
        bg->addChild(label, 5);

        auto button = geode::Button::createWithNode(
            bg,
            [callback = std::move(callback)](geode::Button*) {
                if (callback) callback();
            }
        );
        if (!button) return nullptr;
        button->setPosition(pos);
        button->setAnimationType(geode::Button::AnimationType::Scale);
        parent->addChild(button, 20);
        return button;
    }

    void fitTitle(CCLabelTTF* label, const char* text) {
        if (!label) return;
        label->setString(text);
        const float width = label->getContentSize().width;
        label->setScale(width > m_cardTitleMaxWidth ? m_cardTitleMaxWidth / width : 1.f);
    }

    void buildToggleCard(float cx, float cy, float cardW, float cardH,
                         const char* title, CCLabelTTF*& titleLabel,
                         CCLabelTTF*& statusLabel, CCLabelTTF*& actionLabel,
                         const char* status, std::function<void()> callback) {
        auto card = addCard(cx, cy, cardW, cardH);
        titleLabel = addText(card, title, {cardW * 0.5f, cardH * 0.78f},
                             17.f, ccc3(238, 241, 248), cardW * 0.9f);
        statusLabel = addText(card, status, {cardW * 0.5f, cardH * 0.49f},
                              13.f, ccc3(190, 205, 255));
        addActionButton(card, tr("Toggle", "Переключить"),
                        {cardW * 0.5f, cardH * 0.19f},
                        std::min(118.f, cardW * 0.52f), 27.f,
                        std::move(callback), &actionLabel);
    }

    void buildValueCard(float cx, float cy, float cardW, float cardH,
                        const char* title, CCLabelTTF*& titleLabel,
                        CCLabelTTF*& valueLabel, const char* value,
                        std::function<void()> down, std::function<void()> up) {
        auto card = addCard(cx, cy, cardW, cardH);
        titleLabel = addText(card, title, {cardW * 0.5f, cardH * 0.78f},
                             17.f, ccc3(238, 241, 248), cardW * 0.9f);
        valueLabel = addText(card, value, {cardW * 0.5f, cardH * 0.36f},
                             20.f, ccc3(208, 188, 255));
        addSmallButton(card, "-", {cardW * 0.27f, cardH * 0.34f}, std::move(down));
        addSmallButton(card, "+", {cardW * 0.73f, cardH * 0.34f}, std::move(up));
    }

    void buildRateCard(float cx, float cy, float cardW, float cardH,
                       const char* title, CCLabelTTF*& titleLabel,
                       CCLabelTTF*& statusLabel, CCLabelTTF*& valueLabel,
                       CCLabelTTF*& actionLabel, const char* status,
                       const char* value, std::function<void()> toggle,
                       std::function<void()> down, std::function<void()> up) {
        auto card = addCard(cx, cy, cardW, cardH);
        titleLabel = addText(card, title, {cardW * 0.5f, cardH * 0.82f},
                             16.f, ccc3(238, 241, 248), cardW * 0.9f);
        statusLabel = addText(card, status, {cardW * 0.5f, cardH * 0.62f},
                              11.f, ccc3(190, 205, 255), cardW * 0.85f);
        valueLabel = addText(card, value, {cardW * 0.5f, cardH * 0.37f},
                             15.f, ccc3(208, 188, 255));
        addSmallButton(card, "-", {cardW * 0.23f, cardH * 0.13f}, std::move(down));
        addActionButton(card, tr("Toggle", "Переключить"),
                        {cardW * 0.5f, cardH * 0.13f},
                        std::min(104.f, cardW * 0.42f), 25.f,
                        std::move(toggle), &actionLabel);
        addSmallButton(card, "+", {cardW * 0.77f, cardH * 0.13f}, std::move(up));
    }

    void buildAutoClickerCard(float cx, float cy, float cardW, float cardH) {
        auto card = addCard(cx, cy, cardW, cardH);
        m_autoTitle = addText(card, tr("AUTO CLICKER", "АВТОКЛИКЕР"),
                              {cardW * 0.5f, cardH * 0.83f},
                              16.f, ccc3(238, 241, 248), cardW * 0.9f);
        m_autoStatus = addText(card, "", {cardW * 0.5f, cardH * 0.63f},
                               11.f, ccc3(190, 205, 255));
        addActionButton(card, tr("Toggle", "Переключить"),
                        {cardW * 0.5f, cardH * 0.45f},
                        std::min(98.f, cardW * 0.44f), 23.f,
                        [this]() { onToggleAutoClicker(nullptr); },
                        &m_autoAction);

        m_autoHold = addText(card, "", {cardW * 0.30f, cardH * 0.25f},
                             11.f, ccc3(224, 228, 240));
        m_autoRelease = addText(card, "", {cardW * 0.30f, cardH * 0.10f},
                                11.f, ccc3(224, 228, 240));

        const float minusX = cardW * 0.66f;
        const float plusX = cardW * 0.84f;
        addSmallButton(card, "-", {minusX, cardH * 0.25f},
                       [this]() { onAutoHoldDown(nullptr); });
        addSmallButton(card, "+", {plusX, cardH * 0.25f},
                       [this]() { onAutoHoldUp(nullptr); });
        addSmallButton(card, "-", {minusX, cardH * 0.10f},
                       [this]() { onAutoReleaseDown(nullptr); });
        addSmallButton(card, "+", {plusX, cardH * 0.10f},
                       [this]() { onAutoReleaseUp(nullptr); });
    }

    bool init() {
        auto win = CCDirector::sharedDirector()->getWinSize();
        const float panelW = win.width * 0.94f;
        const float panelH = win.height * 0.92f;
        if (!Popup::init(panelW, panelH, "GJ_square01.png")) return false;

        auto size = m_mainLayer->getContentSize();
        const float w = size.width;
        const float h = size.height;

        // Tint Popup's actual NineSlice background; a layer behind it is hidden.
        if (m_bgSprite) {
            m_bgSprite->setColor(ccc3(42, 88, 190));
            m_bgSprite->setOpacity(255);
        }

        m_titleLabel = addText(m_mainLayer, "GD CONTROL",
                               {w * 0.5f, h * 0.91f}, 25.f,
                               ccc3(238, 241, 248));
        auto langBg = geode::NineSlice::create("GJ_button_02.png", {}, {8.f, 8.f, 8.f, 8.f});
        langBg->setContentSize({70.f, 32.f});
        auto langText = CCLabelTTF::create(isRussian() ? "RU" : "EN", "Arial", 13.f);
        langText->setColor(ccc3(255, 255, 255));
        langText->setPosition({35.f, 16.f});
        langBg->addChild(langText, 5);
        auto langButton = geode::Button::createWithNode(
            langBg,
            [this](geode::Button*) { onToggleLanguage(nullptr); }
        );
        langButton->setPosition({w * 0.84f, h * 0.91f});
        langButton->setAnimationType(geode::Button::AnimationType::Scale);
        m_mainLayer->addChild(langButton, 100);
        m_languageLabel = langText;

        const bool landscape = w >= h * 1.2f;
        const int columns = landscape ? 3 : 2;
        const int rows = (7 + columns - 1) / columns;
        const float marginX = w * 0.035f;
        const float gapX = w * 0.025f;
        const float cardW = (w - marginX * 2.f - gapX * (columns - 1)) / columns;
        const float topCenter = h * 0.75f;
        const float bottomCenter = h * 0.17f;
        const float rowStep = rows > 1 ? (topCenter - bottomCenter) / (rows - 1) : 0.f;
        const float cardH = std::min(
            landscape ? h * 0.205f : h * 0.155f,
            std::max(72.f, rowStep - 12.f)
        );
        m_cardTitleMaxWidth = cardW * 0.88f;

        auto cardCenter = [&](int index) -> CCPoint {
            const int row = index / columns;
            const int col = index % columns;
            float x = marginX + cardW * 0.5f + col * (cardW + gapX);
            if (row == rows - 1 && (7 % columns) == 1) {
                x = w * 0.5f;
            }
            const float y = topCenter - row * rowStep;
            return {x, y};
        };

        const auto p0 = cardCenter(0);
        const auto p1 = cardCenter(1);
        const auto p2 = cardCenter(2);
        const auto p3 = cardCenter(3);
        const auto p4 = cardCenter(4);
        const auto p5 = cardCenter(5);
        const auto p6 = cardCenter(6);

        buildToggleCard(p0.x, p0.y, cardW, cardH, tr("NO-CLIP", "НОУКЛИП"),
                        m_noclipTitle, m_noclipLabel, m_noclipAction,
                        isNoclipEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"),
                        [this]() { onToggleNoclip(nullptr); });
        buildValueCard(p1.x, p1.y, cardW, cardH, tr("SPEEDHACK", "УСКОРЕНИЕ"),
                       m_speedTitle, m_speedLabel,
                       fmt::format("{:.2f}x", getSpeedMultiplier()).c_str(),
                       [this]() { onSpeedDown(nullptr); },
                       [this]() { onSpeedUp(nullptr); });
        buildToggleCard(p2.x, p2.y, cardW, cardH, tr("UNLOCK ICONS", "РАЗБЛОКИРОВКА ИКОНОК"),
                        m_iconsTitle, m_iconsLabel, m_iconsAction,
                        isUnlockIconsEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"),
                        [this]() { onToggleIcons(nullptr); });
        buildToggleCard(p3.x, p3.y, cardW, cardH, tr("PRACTICE MUSIC", "МУЗЫКА В ПРАКТИКЕ"),
                        m_musicTitle, m_musicLabel, m_musicAction,
                        isPracticeMusicEnabled() ? tr("Unlocked", "Разблокировано") : tr("Locked", "Заблокировано"),
                        [this]() { onTogglePracticeMusic(nullptr); });
        buildAutoClickerCard(p4.x, p4.y, cardW, cardH);
        buildRateCard(p5.x, p5.y, cardW, cardH,
                      tr("TPS BYPASS", "ОБХОД TPS"),
                      m_tpsTitle, m_tpsStatus, m_tpsValue, m_tpsAction,
                      isTPSBypassEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"),
                      fmt::format("{} TPS", getTPSBypassValue()).c_str(),
                      [this]() { onToggleTPS(nullptr); },
                      [this]() { onTPSDown(nullptr); },
                      [this]() { onTPSUp(nullptr); });
        buildRateCard(p6.x, p6.y, cardW, cardH,
                      tr("FPS BYPASS", "ОБХОД FPS"),
                      m_fpsTitle, m_fpsStatus, m_fpsValue, m_fpsAction,
                      isFPSBypassEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"),
                      fmt::format("{} FPS", getFPSBypassValue()).c_str(),
                      [this]() { onToggleFPS(nullptr); },
                      [this]() { onFPSDown(nullptr); },
                      [this]() { onFPSUp(nullptr); });

        m_hintLabel = addText(m_mainLayer,
            tr("Local settings · changes apply instantly",
               "Локальные настройки · изменения применяются сразу"),
            {w * 0.5f, h * 0.035f},
            10.f, ccc3(137, 149, 171), w * 0.9f);

        refreshValues();
        return true;
    }

    void refreshValues() {
        if (m_titleLabel) m_titleLabel->setString("GD CONTROL");
        if (m_languageLabel) m_languageLabel->setString(isRussian() ? "RU" : "EN");

        fitTitle(m_noclipTitle, tr("NO-CLIP", "НОУКЛИП"));
        fitTitle(m_speedTitle, tr("SPEEDHACK", "УСКОРЕНИЕ"));
        fitTitle(m_iconsTitle, tr("UNLOCK ICONS", "РАЗБЛОКИРОВКА ИКОНОК"));
        fitTitle(m_musicTitle, tr("PRACTICE MUSIC", "МУЗЫКА В ПРАКТИКЕ"));
        fitTitle(m_autoTitle, tr("AUTO CLICKER", "АВТОКЛИКЕР"));
        fitTitle(m_tpsTitle, tr("TPS BYPASS", "ОБХОД TPS"));
        fitTitle(m_fpsTitle, tr("FPS BYPASS", "ОБХОД FPS"));

        if (m_noclipLabel) m_noclipLabel->setString(isNoclipEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"));
        if (m_speedLabel) m_speedLabel->setString(fmt::format("{:.2f}x", getSpeedMultiplier()).c_str());
        if (m_iconsLabel) m_iconsLabel->setString(isUnlockIconsEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"));
        if (m_musicLabel) m_musicLabel->setString(isPracticeMusicEnabled() ? tr("Unlocked", "Разблокировано") : tr("Locked", "Заблокировано"));

        if (m_autoStatus) m_autoStatus->setString(isAutoClickerEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"));
        if (m_autoHold) m_autoHold->setString(fmt::format("{}: {}", tr("HOLD", "НАЖАТИЕ"), getAutoClickerHold()).c_str());
        if (m_autoRelease) m_autoRelease->setString(fmt::format("{}: {}", tr("RELEASE", "ОТПУСК"), getAutoClickerRelease()).c_str());

        if (m_tpsStatus) m_tpsStatus->setString(isTPSBypassEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"));
        if (m_tpsValue) m_tpsValue->setString(fmt::format("{} TPS", getTPSBypassValue()).c_str());
        if (m_fpsStatus) m_fpsStatus->setString(isFPSBypassEnabled() ? tr("Enabled", "Включено") : tr("Disabled", "Выключено"));
        if (m_fpsValue) m_fpsValue->setString(fmt::format("{} FPS", getFPSBypassValue()).c_str());

        if (m_noclipAction) m_noclipAction->setString(tr("Toggle", "Переключить"));
        if (m_iconsAction) m_iconsAction->setString(tr("Toggle", "Переключить"));
        if (m_musicAction) m_musicAction->setString(tr("Toggle", "Переключить"));
        if (m_autoAction) m_autoAction->setString(tr("Toggle", "Переключить"));
        if (m_tpsAction) m_tpsAction->setString(tr("Toggle", "Переключить"));
        if (m_fpsAction) m_fpsAction->setString(tr("Toggle", "Переключить"));
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

    void onSpeedDown(CCObject*) { setSpeedMultiplier(getSpeedMultiplier() - 0.05f); refreshValues(); }
    void onSpeedUp(CCObject*) { setSpeedMultiplier(getSpeedMultiplier() + 0.05f); refreshValues(); }

    void onToggleAutoClicker(CCObject*) {
        Mod::get()->setSavedValue<bool>("autoclicker-enabled", !isAutoClickerEnabled());
        if (!isAutoClickerEnabled()) stopAutoClicker(PlayLayer::get());
        refreshValues();
    }
    void onAutoHoldDown(CCObject*) {
        Mod::get()->setSavedValue<int64_t>("autoclicker-hold", std::max<int64_t>(1, getAutoClickerHold() - 1));
        refreshValues();
    }
    void onAutoHoldUp(CCObject*) {
        Mod::get()->setSavedValue<int64_t>("autoclicker-hold", std::min<int64_t>(20, getAutoClickerHold() + 1));
        refreshValues();
    }
    void onAutoReleaseDown(CCObject*) {
        Mod::get()->setSavedValue<int64_t>("autoclicker-release", std::max<int64_t>(1, getAutoClickerRelease() - 1));
        refreshValues();
    }
    void onAutoReleaseUp(CCObject*) {
        Mod::get()->setSavedValue<int64_t>("autoclicker-release", std::min<int64_t>(20, getAutoClickerRelease() + 1));
        refreshValues();
    }

    void onToggleTPS(CCObject*) {
        Mod::get()->setSavedValue<bool>("tps-bypass-enabled", !isTPSBypassEnabled());
        refreshValues();
    }
    void onTPSDown(CCObject*) {
        Mod::get()->setSavedValue<int64_t>("tps-bypass-value", std::max<int64_t>(30, getTPSBypassValue() - 30));
        refreshValues();
    }
    void onTPSUp(CCObject*) {
        Mod::get()->setSavedValue<int64_t>("tps-bypass-value", std::min<int64_t>(2400, getTPSBypassValue() + 30));
        refreshValues();
    }

    void onToggleFPS(CCObject*) {
        Mod::get()->setSavedValue<bool>("fps-bypass-enabled", !isFPSBypassEnabled());
        applyFPSBypass();
        refreshValues();
    }
    void onFPSDown(CCObject*) {
        Mod::get()->setSavedValue<int64_t>("fps-bypass-value", std::max<int64_t>(30, getFPSBypassValue() - 10));
        applyFPSBypass();
        refreshValues();
    }
    void onFPSUp(CCObject*) {
        Mod::get()->setSavedValue<int64_t>("fps-bypass-value", std::min<int64_t>(1000, getFPSBypassValue() + 10));
        applyFPSBypass();
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

        if (isFPSBypassEnabled()) applyFPSBypass();

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
    void pauseGame(bool paused) {
        if (paused) stopAutoClicker(this);
        PlayLayer::pauseGame(paused);
    }

    void resetLevel() {
        stopAutoClicker(this);
        PlayLayer::resetLevel();
    }

    void onQuit() {
        stopAutoClicker(this);
        PlayLayer::onQuit();
    }

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

class $modify(GDControlAutoClicker, GJBaseGameLayer) {
    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        auto layer = PlayLayer::get();
        if (!isAutoClickerEnabled() || !layer || layer->m_playerDied ||
            layer->m_levelEndAnimationStarted) {
            stopAutoClicker(this);
            return;
        }

        const int holdFrames = getAutoClickerHold();
        const int releaseFrames = getAutoClickerRelease();

        if (!g_autoClickerStarted) {
            g_autoClickerInjected = true;
            GJBaseGameLayer::handleButton(true, 1, true);
            g_autoClickerInjected = false;
            g_autoClickerHeld = true;
            g_autoClickerStarted = true;
            g_autoClickerCounter = 0;
            return;
        }

        if (g_autoClickerHeld) {
            if (++g_autoClickerCounter >= holdFrames) {
                g_autoClickerInjected = true;
                GJBaseGameLayer::handleButton(false, 1, true);
                g_autoClickerInjected = false;
                g_autoClickerHeld = false;
                g_autoClickerCounter = 0;
            }
        } else if (++g_autoClickerCounter >= releaseFrames) {
            g_autoClickerInjected = true;
            GJBaseGameLayer::handleButton(true, 1, true);
            g_autoClickerInjected = false;
            g_autoClickerHeld = true;
            g_autoClickerCounter = 0;
        }
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        if (button == 1 && isPlayer1 && isAutoClickerEnabled() && !g_autoClickerInjected) {
            if (!down) GJBaseGameLayer::handleButton(false, button, isPlayer1);
            return;
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
};

class $modify(GDControlTPSBypass, GJBaseGameLayer) {
    struct Fields {
        float m_leftoverDelta = 0.f;
    };

    void update(float dt) {
        if (!isTPSBypassEnabled() || getTPSBypassValue() == 240 ||
            !PlayLayer::get() || !m_started) {
            m_fields->m_leftoverDelta = 0.f;
            return GJBaseGameLayer::update(dt);
        }

        const float newDelta = 1.f / static_cast<float>(getTPSBypassValue());
        float realDelta = dt + m_fields->m_leftoverDelta;
        if (realDelta > dt && newDelta < dt) realDelta = dt;

        int steps = std::clamp(static_cast<int>(realDelta / newDelta), 0, 256);
        if (steps <= 0) {
            m_fields->m_leftoverDelta = realDelta;
            return;
        }

        const auto started = std::chrono::steady_clock::now();
        int completed = 0;
        for (; completed < steps; ++completed) {
            GJBaseGameLayer::update(newDelta);
            if (std::chrono::steady_clock::now() - started >
                std::chrono::duration<double, std::milli>(14.0)) {
                ++completed;
                break;
            }
        }

        m_fields->m_leftoverDelta += dt - newDelta * completed;
    }

    double getModifiedDelta(float dt) {
        if (!isTPSBypassEnabled() || getTPSBypassValue() == 240 ||
            !PlayLayer::get()) {
            return GJBaseGameLayer::getModifiedDelta(dt);
        }

        if (m_resumeTimer > 0) {
            --m_resumeTimer;
            dt = 0.f;
        }

        double timeWarp = std::min<double>(m_gameState.m_timeWarp, 1.0);
        if (timeWarp <= 0.0) timeWarp = 1.0;

        const double fixedDelta = (1.0 / static_cast<double>(getTPSBypassValue())) * timeWarp;
        const double accumulated = static_cast<double>(dt) + m_extraDelta;
        const double steps = std::round(accumulated / fixedDelta);
        const double adjustedDelta = steps * fixedDelta;
        m_extraDelta = accumulated - adjustedDelta;
        return adjustedDelta;
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

    void updateCustomFPS() {
        GameManager::updateCustomFPS();
        if (isFPSBypassEnabled()) {
            CCDirector::sharedDirector()->setAnimationInterval(
                1.0 / static_cast<double>(getFPSBypassValue())
            );
        }
    }
};

$on_mod(Loaded) {
    log::info("GD Control v0.2.0 loaded for Geometry Dash 2.2091.");
    if (isFPSBypassEnabled()) applyFPSBypass();
}
