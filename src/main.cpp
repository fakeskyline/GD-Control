#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/Button.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/CCScene.hpp>
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
    CCLabelTTF* m_autoHoldTitle = nullptr;
    CCLabelTTF* m_autoReleaseTitle = nullptr;
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
    CCNode* m_pages[3] = { nullptr, nullptr, nullptr };
    CCLabelTTF* m_tabLabels[3] = { nullptr, nullptr, nullptr };
    CCNode* m_tabBgs[3] = { nullptr, nullptr, nullptr };
    CCLabelTTF* m_noclipCheck = nullptr;
    CCLabelTTF* m_iconsCheck = nullptr;
    CCLabelTTF* m_musicCheck = nullptr;
    CCLabelTTF* m_autoCheck = nullptr;
    CCLabelTTF* m_tpsCheck = nullptr;
    CCLabelTTF* m_fpsCheck = nullptr;
    int m_activePage = 0;

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
        card->setAnchorPoint({0.5f, 0.5f});
        card->setPosition({centerX, centerY});
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
        constexpr float buttonW = 22.f;
        constexpr float buttonH = 22.f;
        bg->setContentSize({buttonW, buttonH});

        auto label = CCLabelTTF::create(text, "Arial", 13.f);
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
                              {cardW * 0.5f, cardH * 0.84f},
                              16.f, ccc3(238, 241, 248), cardW * 0.9f);
        m_autoStatus = addText(card, "", {cardW * 0.5f, cardH * 0.65f},
                               11.f, ccc3(190, 205, 255));
        addActionButton(card, tr("Toggle", "Переключить"),
                        {cardW * 0.5f, cardH * 0.49f},
                        std::min(96.f, cardW * 0.42f), 22.f,
                        [this]() { onToggleAutoClicker(nullptr); },
                        &m_autoAction);

        m_autoHold = addText(card, "", {cardW * 0.32f, cardH * 0.29f},
                             10.f, ccc3(224, 228, 240));
        m_autoRelease = addText(card, "", {cardW * 0.32f, cardH * 0.08f},
                                10.f, ccc3(224, 228, 240));

        const float minusX = cardW * 0.70f;
        const float plusX = cardW * 0.87f;
        addSmallButton(card, "-", {minusX, cardH * 0.29f},
                       [this]() { onAutoHoldDown(nullptr); });
        addSmallButton(card, "+", {plusX, cardH * 0.29f},
                       [this]() { onAutoHoldUp(nullptr); });
        addSmallButton(card, "-", {minusX, cardH * 0.08f},
                       [this]() { onAutoReleaseDown(nullptr); });
        addSmallButton(card, "+", {plusX, cardH * 0.08f},
                       [this]() { onAutoReleaseUp(nullptr); });
    }

    void setPage(int index) {
        if (index < 0 || index >= 3) return;
        m_activePage = index;
        for (int i = 0; i < 3; ++i) {
            if (m_pages[i]) m_pages[i]->setVisible(i == index);
            if (m_tabLabels[i]) m_tabLabels[i]->setColor(i == index
                ? ccc3(245, 247, 252) : ccc3(170, 179, 195));
            if (m_tabBgs[i]) m_tabBgs[i]->setColor(i == index
                ? ccc3(54, 78, 124) : ccc3(38, 42, 52));
        }
    }

    bool init() {
        constexpr float panelW = 480.f;
        constexpr float panelH = 280.f;
        if (!Popup::init(panelW, panelH, "GJ_square01.png")) return false;

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto size = m_mainLayer->getContentSize();
        const float w = size.width;
        const float h = size.height;

        this->setTitle("");
        if (m_buttonMenu) m_buttonMenu->setContentSize(win);
        if (m_bgSprite) m_bgSprite->removeMeAndCleanup();

        // Eclipse Popup composition: NineSlice frame, sidebar tabs, and a content view.
        auto border = geode::NineSlice::create("square02b_001.png");
        border->setContentSize({w, h});
        border->setColor(ccc3(12, 15, 24));
        m_mainLayer->addChildAtPosition(border, geode::Anchor::Center, {0.f, 0.f}, -2);

        auto background = geode::NineSlice::create("square02b_001.png");
        background->setContentSize({w - 4.f, h - 4.f});
        background->setColor(ccc3(27, 29, 36));
        m_mainLayer->addChildAtPosition(background, geode::Anchor::Center, {0.f, 0.f}, -1);

        m_titleLabel = addText(m_mainLayer, "GD CONTROL",
            {w * 0.5f, h - 23.f}, 22.f, ccc3(239, 241, 247));

        auto langBG = geode::NineSlice::create("square02b_001.png", {0.f, 0.f, 80.f, 80.f});
        langBG->setContentSize({42.f, 25.f});
        langBG->setColor(ccc3(48, 68, 105));
        auto langText = CCLabelTTF::create(isRussian() ? "RU" : "EN", "Arial", 11.f);
        langText->setColor(ccc3(240, 243, 250));
        langText->setPosition({21.f, 12.5f});
        langBG->addChild(langText, 2);
        auto langButton = geode::Button::createWithNode(
            langBG, [this](geode::Button*) { onToggleLanguage(nullptr); });
        langButton->setPosition({w - 34.f, h - 23.f});
        langButton->setAnimationType(geode::Button::AnimationType::Scale);
        m_mainLayer->addChild(langButton, 50);
        m_languageLabel = langText;

        const float contentTop = h - 43.f;
        const float contentBottom = 10.f;
        const float contentH = contentTop - contentBottom;
        constexpr float sideX = 6.f;
        constexpr float sideW = 115.f;
        constexpr float contentX = 125.f;
        constexpr float contentW = 345.f;

        auto sideBG = geode::NineSlice::create("square02b_001.png");
        sideBG->setAnchorPoint({0.f, 0.5f});
        sideBG->setContentSize({sideW, contentH});
        sideBG->setColor(ccc3(34, 37, 45));
        sideBG->setPosition({sideX, (contentTop + contentBottom) * 0.5f});
        m_mainLayer->addChild(sideBG, 1);

        auto contentBG = geode::NineSlice::create("square02b_001.png");
        contentBG->setAnchorPoint({0.f, 1.f});
        contentBG->setContentSize({contentW, contentH});
        contentBG->setColor(ccc3(25, 28, 35));
        contentBG->setPosition({contentX, contentTop});
        m_mainLayer->addChild(contentBG, 1);

        const char* navEn[3] = {"PLAYER", "UNLOCKS", "BYPASS"};
        const char* navRu[3] = {"ИГРОК", "РАЗБЛОКИРОВКИ", "ОБХОДЫ"};
        for (int i = 0; i < 3; ++i) {
            auto tabNode = CCNode::create();
            tabNode->setContentSize({120.f, 28.f});
            auto tabBG = geode::NineSlice::create("square02b_001.png", {0.f, 0.f, 80.f, 80.f});
            tabBG->setContentSize({120.f, 36.f});
            tabBG->setScaleY(0.75f);
            tabNode->addChildAtPosition(tabBG, geode::Anchor::Center);

            auto label = CCLabelTTF::create(isRussian() ? navRu[i] : navEn[i], "Arial", 10.f);
            label->setColor(i == 0 ? ccc3(245, 247, 252) : ccc3(170, 179, 195));
            if (label->getContentSize().width > 108.f)
                label->setScale(108.f / label->getContentSize().width);
            tabNode->addChildAtPosition(label, geode::Anchor::Center);

            auto button = geode::Button::createWithNode(
                tabNode, [this, i](geode::Button*) { setPage(i); });
            button->setPosition({sideX + sideW * 0.5f, contentTop - 35.f - i * 42.f});
            button->setAnimationType(geode::Button::AnimationType::Scale);
            m_mainLayer->addChild(button, 20);
            m_tabLabels[i] = label;
            m_tabBgs[i] = tabBG;
        }

        for (int i = 0; i < 3; ++i) {
            m_pages[i] = CCNode::create();
            m_pages[i]->setContentSize({contentW, contentH});
            m_pages[i]->setAnchorPoint({0.f, 1.f});
            m_pages[i]->setPosition({contentX, contentTop});
            m_mainLayer->addChild(m_pages[i], 5);
        }

        auto addComponentRow = [&](CCNode* page, int index, const char* title,
                                   CCLabelTTF*& titleOut, CCLabelTTF*& checkOut,
                                   bool checked, std::function<void()> toggle) -> CCNode* {
            constexpr float rowHeight = 28.f;
            constexpr float rowGap = 2.5f;
            const float centerY = contentH - 17.f - index * (rowHeight + rowGap);

            auto row = CCNode::create();
            row->setContentSize({contentW, rowHeight});
            row->setPosition({0.f, centerY - rowHeight * 0.5f});
            page->addChild(row);

            auto checkBG = geode::NineSlice::create("square02b_001.png", {0.f, 0.f, 80.f, 80.f});
            checkBG->setContentSize({15.f, 15.f});
            checkBG->setColor(ccc3(43, 47, 58));
            auto checkText = CCLabelTTF::create(checked ? "✓" : "", "Arial", 11.f);
            checkText->setColor(ccc3(106, 218, 232));
            checkText->setPosition({7.5f, 7.5f});
            checkBG->addChild(checkText, 2);

            auto checkButton = geode::Button::createWithNode(
                checkBG, [toggle = std::move(toggle)](geode::Button*) { if (toggle) toggle(); });
            checkButton->setPosition({15.f, rowHeight * 0.5f});
            checkButton->setAnimationType(geode::Button::AnimationType::Scale);
            row->addChild(checkButton, 5);
            checkOut = checkText;

            titleOut = addText(row, title, {29.f, rowHeight * 0.5f},
                               11.f, ccc3(224, 228, 237), 190.f);
            titleOut->setAnchorPoint({0.f, 0.5f});
            return row;
        };

        auto addPlainRow = [&](CCNode* page, int index, const char* title,
                               CCLabelTTF*& titleOut) -> CCNode* {
            constexpr float rowHeight = 28.f;
            constexpr float rowGap = 2.5f;
            const float centerY = contentH - 17.f - index * (rowHeight + rowGap);
            auto row = CCNode::create();
            row->setContentSize({contentW, rowHeight});
            row->setPosition({0.f, centerY - rowHeight * 0.5f});
            page->addChild(row);
            titleOut = addText(row, title, {15.f, rowHeight * 0.5f},
                10.f, ccc3(174, 184, 201), 190.f);
            titleOut->setAnchorPoint({0.f, 0.5f});
            return row;
        };

        auto addValueControls = [&](CCNode* row, CCLabelTTF*& valueOut, const char* value,
                                    std::function<void()> down, std::function<void()> up,
                                    float valueX = 260.f) {
            valueOut = addText(row, value, {valueX, 14.f}, 11.f, ccc3(202, 187, 255), 82.f);
            addSmallButton(row, "-", {valueX - 42.f, 14.f}, std::move(down));
            addSmallButton(row, "+", {valueX + 43.f, 14.f}, std::move(up));
        };

        // PLAYER: toggle rows and compact numeric component rows.
        addComponentRow(m_pages[0], 0, tr("No-Clip", "Ноуклип"),
            m_noclipTitle, m_noclipCheck, isNoclipEnabled(),
            [this]() { onToggleNoclip(nullptr); });

        auto speedRow = addPlainRow(m_pages[0], 1, tr("Speedhack", "Ускорение"), m_speedTitle);
        addValueControls(speedRow, m_speedLabel,
            fmt::format("{:.2f}x", getSpeedMultiplier()).c_str(),
            [this]() { onSpeedDown(nullptr); }, [this]() { onSpeedUp(nullptr); }, 260.f);

        addComponentRow(m_pages[0], 2, tr("Auto Clicker", "Автокликер"),
            m_autoTitle, m_autoCheck, isAutoClickerEnabled(),
            [this]() { onToggleAutoClicker(nullptr); });

        auto holdRow = addPlainRow(m_pages[0], 3, tr("Press", "Нажатие"), m_autoHoldTitle);
        addValueControls(holdRow, m_autoHold, fmt::format("{}", getAutoClickerHold()).c_str(),
            [this]() { onAutoHoldDown(nullptr); }, [this]() { onAutoHoldUp(nullptr); }, 260.f);

        auto releaseRow = addPlainRow(m_pages[0], 4, tr("Release", "Отпускание"), m_autoReleaseTitle);
        addValueControls(releaseRow, m_autoRelease, fmt::format("{}", getAutoClickerRelease()).c_str(),
            [this]() { onAutoReleaseDown(nullptr); }, [this]() { onAutoReleaseUp(nullptr); }, 260.f);

        // UNLOCKS
        addComponentRow(m_pages[1], 0, tr("Unlock Icons", "Разблокировка иконок"),
            m_iconsTitle, m_iconsCheck, isUnlockIconsEnabled(),
            [this]() { onToggleIcons(nullptr); });
        addComponentRow(m_pages[1], 1, tr("Practice Music", "Музыка в практике"),
            m_musicTitle, m_musicCheck, isPracticeMusicEnabled(),
            [this]() { onTogglePracticeMusic(nullptr); });

        // BYPASS
        auto tpsRow = addComponentRow(m_pages[2], 0, tr("TPS Bypass", "Обход TPS"),
            m_tpsTitle, m_tpsCheck, isTPSBypassEnabled(),
            [this]() { onToggleTPS(nullptr); });
        addValueControls(tpsRow, m_tpsValue, fmt::format("{} TPS", getTPSBypassValue()).c_str(),
            [this]() { onTPSDown(nullptr); }, [this]() { onTPSUp(nullptr); }, 260.f);

        auto fpsRow = addComponentRow(m_pages[2], 1, tr("FPS Bypass", "Обход FPS"),
            m_fpsTitle, m_fpsCheck, isFPSBypassEnabled(),
            [this]() { onToggleFPS(nullptr); });
        addValueControls(fpsRow, m_fpsValue, fmt::format("{} FPS", getFPSBypassValue()).c_str(),
            [this]() { onFPSDown(nullptr); }, [this]() { onFPSUp(nullptr); }, 260.f);

        m_hintLabel = addText(m_mainLayer,
            tr("Local settings · changes apply instantly",
               "Локальные настройки · изменения применяются сразу"),
            {w * 0.5f, 5.f}, 8.f, ccc3(137, 149, 171), w * 0.72f);

        setPage(0);
        refreshValues();
        return true;
    }

    void refreshValues() {
        if (m_titleLabel) m_titleLabel->setString("GD CONTROL");
        if (m_languageLabel) m_languageLabel->setString(isRussian() ? "RU" : "EN");
        if (m_tabLabels[0]) m_tabLabels[0]->setString(tr("PLAYER", "ИГРОК"));
        if (m_tabLabels[1]) m_tabLabels[1]->setString(tr("UNLOCKS", "РАЗБЛОКИРОВКИ"));
        if (m_tabLabels[2]) m_tabLabels[2]->setString(tr("BYPASS", "ОБХОДЫ"));

        if (m_noclipTitle) m_noclipTitle->setString(tr("No-Clip", "Ноуклип"));
        if (m_speedTitle) m_speedTitle->setString(tr("Speedhack", "Ускорение"));
        if (m_autoTitle) m_autoTitle->setString(tr("Auto Clicker", "Автокликер"));
        if (m_autoHoldTitle) m_autoHoldTitle->setString(tr("Press", "Нажатие"));
        if (m_autoReleaseTitle) m_autoReleaseTitle->setString(tr("Release", "Отпускание"));
        if (m_iconsTitle) m_iconsTitle->setString(tr("Unlock Icons", "Разблокировка иконок"));
        if (m_musicTitle) m_musicTitle->setString(tr("Practice Music", "Музыка в практике"));
        if (m_tpsTitle) m_tpsTitle->setString(tr("TPS Bypass", "Обход TPS"));
        if (m_fpsTitle) m_fpsTitle->setString(tr("FPS Bypass", "Обход FPS"));

        if (m_noclipCheck) m_noclipCheck->setString(isNoclipEnabled() ? "✓" : "");
        if (m_iconsCheck) m_iconsCheck->setString(isUnlockIconsEnabled() ? "✓" : "");
        if (m_musicCheck) m_musicCheck->setString(isPracticeMusicEnabled() ? "✓" : "");
        if (m_autoCheck) m_autoCheck->setString(isAutoClickerEnabled() ? "✓" : "");
        if (m_tpsCheck) m_tpsCheck->setString(isTPSBypassEnabled() ? "✓" : "");
        if (m_fpsCheck) m_fpsCheck->setString(isFPSBypassEnabled() ? "✓" : "");

        if (m_speedLabel) m_speedLabel->setString(fmt::format("{:.2f}x", getSpeedMultiplier()).c_str());
        if (m_autoHold) m_autoHold->setString(fmt::format("{}", getAutoClickerHold()).c_str());
        if (m_autoRelease) m_autoRelease->setString(fmt::format("{}", getAutoClickerRelease()).c_str());
        if (m_tpsValue) m_tpsValue->setString(fmt::format("{} TPS", getTPSBypassValue()).c_str());
        if (m_fpsValue) m_fpsValue->setString(fmt::format("{} FPS", getFPSBypassValue()).c_str());

        if (m_hintLabel) m_hintLabel->setString(tr(
            "Local settings · changes apply instantly",
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
class GDFloatingButton : public CCMenu, public cocos2d::CCTargetedTouchDelegate {
protected:
    ButtonSprite* m_sprite = nullptr;
    bool m_dragging = false;
    bool m_moved = false;
    CCPoint m_touchStart = CCPointZero;
    CCPoint m_spriteStart = CCPointZero;

    bool init() {
        if (!CCMenu::init()) return false;
        auto win = CCDirector::sharedDirector()->getWinSize();
        setContentSize(win);
        setPosition(CCPointZero);
        setAnchorPoint(CCPointZero);

        m_sprite = ButtonSprite::create("GDC", "bigFont.fnt", "GJ_button_01.png", 0.65f);
        if (!m_sprite) return false;
        m_sprite->setAnchorPoint({0.5f, 0.5f});

        const float x = Mod::get()->getSavedValue<float>("floating-button-x", win.width - 48.f);
        const float y = Mod::get()->getSavedValue<float>("floating-button-y", 48.f);
        const float halfW = m_sprite->getContentSize().width * 0.5f;
        const float halfH = m_sprite->getContentSize().height * 0.5f;
        m_sprite->setPosition({
            std::clamp(x, halfW, win.width - halfW),
            std::clamp(y, halfH, win.height - halfH)
        });
        addChild(m_sprite, 1);
        setID("gd-control-global-floating-button"_spr);
        return true;
    }

    void onEnter() override {
        CCMenu::onEnter();
        CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -1000, true);
    }

    void onExit() override {
        CCDirector::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
        CCMenu::onExit();
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (!isVisible() || !m_sprite) return false;
        auto point = convertToNodeSpace(touch->getLocation());
        auto pos = m_sprite->getPosition();
        auto size = m_sprite->getContentSize();
        CCRect bounds(pos.x - size.width * 0.5f, pos.y - size.height * 0.5f,
                      size.width, size.height);
        if (!bounds.containsPoint(point)) return false;
        m_dragging = true;
        m_moved = false;
        m_touchStart = point;
        m_spriteStart = pos;
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!m_dragging || !m_sprite) return;
        auto point = convertToNodeSpace(touch->getLocation());
        auto delta = point - m_touchStart;
        if (std::fabs(delta.x) + std::fabs(delta.y) > 7.f) m_moved = true;
        if (!m_moved) return;

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto size = m_sprite->getContentSize();
        const float halfW = size.width * 0.5f;
        const float halfH = size.height * 0.5f;
        auto target = m_spriteStart + delta;
        target.x = std::clamp(target.x, halfW, win.width - halfW);
        target.y = std::clamp(target.y, halfH, win.height - halfH);
        m_sprite->setPosition(target);
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (!m_dragging) return;
        m_dragging = false;
        if (m_moved) {
            Mod::get()->setSavedValue<float>("floating-button-x", m_sprite->getPositionX());
            Mod::get()->setSavedValue<float>("floating-button-y", m_sprite->getPositionY());
            return;
        }
        if (auto popup = GDControlPopup::create()) popup->show();
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

static GDFloatingButton* g_floatingButton = nullptr;

static void installFloatingButton() {
    if (g_floatingButton) return;
    auto button = GDFloatingButton::create();
    if (!button) return;
    geode::OverlayManager::get()->addChild(button);
    g_floatingButton = button;
}

class $modify(GDControlMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (isFPSBypassEnabled()) applyFPSBypass();
        return true;
    }
};

class $modify(GDControlPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool dontCreateObjects) {
        if (!PlayLayer::init(level, dontCreateObjects)) return false;
        return true;
    }

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

        if (player == m_player1 || player == m_player2) return;
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
    log::info("GD Control v0.2.1 loaded for Geometry Dash 2.2091.");
    installFloatingButton();
    if (isFPSBypassEnabled()) applyFPSBypass();
}
