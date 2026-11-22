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

bool gdControlSwitchStartPos(int direction);

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
    bool isUnlockColorsEnabled() {
        return Mod::get()->getSettingValue<bool>("unlock-colors");
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

class GDControlPopup;
static GDControlPopup* g_activePopup = nullptr;

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
    CCLabelTTF* m_instantCompleteTitle = nullptr;
    CCLabelTTF* m_colorsTitle = nullptr;

    float m_cardTitleMaxWidth = 0.f;
    CCNode* m_pages[4] = { nullptr, nullptr, nullptr, nullptr };
    CCLabelTTF* m_tabLabels[4] = { nullptr, nullptr, nullptr, nullptr };
    geode::NineSlice* m_tabBgs[4] = { nullptr, nullptr, nullptr, nullptr };
    CCMenuItemSpriteExtra* m_tabButtons[4] = { nullptr, nullptr, nullptr, nullptr };
    CCMenu* m_tabMenu = nullptr;
    geode::NineSlice* m_contentBG = nullptr;
    CCLabelTTF* m_noclipCheck = nullptr;
    CCLabelTTF* m_iconsCheck = nullptr;
    CCLabelTTF* m_colorsCheck = nullptr;
    CCLabelTTF* m_musicCheck = nullptr;
    CCLabelTTF* m_autoCheck = nullptr;
    CCLabelTTF* m_tpsCheck = nullptr;
    CCLabelTTF* m_fpsCheck = nullptr;
    int m_activePage = 0;

    bool isRussian() {
        return Mod::get()->getSavedValue<bool>("ui-russian", true);
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

    void onTabButton(CCObject* sender) {
        if (sender) setPage(sender->getTag());
    }

    void setPage(int index) {
        if (index < 0 || index >= 4) return;
        m_activePage = index;
        for (int i = 0; i < 4; ++i) {
            const bool active = i == index;
            if (m_pages[i]) m_pages[i]->setVisible(active);
            if (m_tabButtons[i]) m_tabButtons[i]->setEnabled(!active);
            if (m_tabLabels[i]) m_tabLabels[i]->setColor(active
                ? ccc3(36, 39, 58) : ccc3(202, 211, 245));
            if (m_tabBgs[i]) m_tabBgs[i]->setColor(active
                ? ccc3(202, 211, 245) : ccc3(54, 58, 79));
        }
    }

    bool init() {
        constexpr float panelW = 480.f;
        constexpr float panelH = 280.f;
        if (!Popup::init(panelW, panelH)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto size = m_mainLayer->getContentSize();

        this->setTitle("");
        m_buttonMenu->setContentSize(winSize);
        m_closeBtn->setPosition(20.f, winSize.height - 20.f);
        m_bgSprite->removeMeAndCleanup();

        auto bgBehind = geode::NineSlice::create("square02b_001.png");
        bgBehind->setContentSize(size * 0.5f);
        bgBehind->setColor(ccc3(30, 32, 48));
        bgBehind->setID("bg-behind");
        m_mainLayer->addChildAtPosition(bgBehind, geode::Anchor::Center);

        m_bgSprite = geode::NineSlice::create("square02b_001.png");
        m_bgSprite->setContentSize(size - 3.f);
        m_bgSprite->setColor(ccc3(30, 32, 48));
        m_bgSprite->setID("main-bg");
        m_mainLayer->addChildAtPosition(m_bgSprite, geode::Anchor::Center);

        m_contentBG = geode::NineSlice::create("square02b_001.png");
        m_contentBG->setAnchorPoint({0.f, 1.f});
        m_contentBG->setPosition(125.f, 270.f);
        m_contentBG->setColor(ccc3(36, 39, 58));
        m_contentBG->setContentSize({345.f, 260.f});
        m_contentBG->setID("content-bg");
        m_mainLayer->addChild(m_contentBG);

        auto bgTab = geode::NineSlice::create("square02b_001.png");
        bgTab->setAnchorPoint({0.f, 0.5f});
        bgTab->setContentSize({115.f, 260.f});
        bgTab->setColor(ccc3(36, 39, 58));
        bgTab->setPosition(6.f, 140.f);
        bgTab->setID("tab-bg");
        m_mainLayer->addChild(bgTab);

        constexpr float contentW = 345.f;
        constexpr float contentH = 260.f;

        m_tabMenu = CCMenu::create();
        m_tabMenu->setContentSize({120.f, 260.f});
        m_tabMenu->setAnchorPoint({0.5f, 0.5f});
        const char* navEn[4] = {"PLAYER", "UNLOCKS", "BYPASS", "LEVEL"};
        const char* navRu[4] = {"ИГРОК", "РАЗБЛОКИРОВКИ", "ОБХОДЫ", "УРОВЕНЬ"};

        for (int i = 0; i < 4; ++i) {
            auto tabNode = CCNode::create();
            tabNode->setScale(0.9f);
            tabNode->setContentSize({120.f, 28.f});

            auto tabBG = geode::NineSlice::create("square02b_001.png", {0.f, 0.f, 80.f, 80.f});
            tabBG->setContentSize({120.f, 36.f});
            tabBG->setScaleY(0.75f);
            tabNode->addChildAtPosition(tabBG, geode::Anchor::Center);

            auto label = CCLabelTTF::create(isRussian() ? navRu[i] : navEn[i], "Arial", 12.f);
            label->setColor(ccc3(202, 211, 245));
            label->setAnchorPoint({0.5f, 0.5f});
            if (label->getContentSize().width > 100.f)
                label->setScale(100.f / label->getContentSize().width);
            tabNode->addChildAtPosition(label, geode::Anchor::Center);

            auto button = CCMenuItemSpriteExtra::create(
                tabNode, this, menu_selector(GDControlPopup::onTabButton)
            );
            button->setTag(i);
            m_tabMenu->addChild(button);

            m_tabLabels[i] = label;
            m_tabBgs[i] = tabBG;
            m_tabButtons[i] = button;
        }

        m_tabMenu->setLayout(
            geode::AxisLayout::create(geode::Axis::Column)
                ->setAutoScale(false)
                ->setAxisReverse(true)
                ->setGrowCrossAxis(false)
                ->setCrossAxisOverflow(true)
                ->setGap(0.5f)
                ->setAxisAlignment(geode::AxisAlignment::End)
                ->setCrossAxisAlignment(geode::AxisAlignment::Start)
                ->setCrossAxisLineAlignment(geode::AxisAlignment::Center)
                ->ignoreInvisibleChildren(false),
            true
        );
        m_tabMenu->setContentHeight(260.f);
        m_tabMenu->updateLayout();
        bgTab->addChildAtPosition(m_tabMenu, geode::Anchor::Center, {0.f, -20.f});

        for (int i = 0; i < 4; ++i) {
            m_pages[i] = CCNode::create();
            m_pages[i]->setContentSize({contentW, contentH});
            m_pages[i]->setAnchorPoint({0.f, 1.f});
            m_pages[i]->setPosition(125.f, 270.f);
            m_mainLayer->addChild(m_pages[i], 1);
        }

        auto addComponentRow = [&](CCNode* page, int index, const char* title,
                                   CCLabelTTF*& titleOut, CCLabelTTF*& checkOut,
                                   bool checked, std::function<void()> toggle) -> CCNode* {
            constexpr float rowHeight = 28.f;
            constexpr float rowGap = 2.5f;
            const float centerY = contentH - 29.f - index * (rowHeight + rowGap);

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
            const float centerY = contentH - 29.f - index * (rowHeight + rowGap);
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

        auto instantRow = addPlainRow(m_pages[0], 5,
            tr("Instant Complete", "Мгновенное прохождение"), m_instantCompleteTitle);
        addActionButton(instantRow, tr("Complete", "Пройти"),
            {contentW - 62.f, 14.f}, 104.f, 22.f,
            [this]() { onInstantComplete(nullptr); });

        // UNLOCKS
        addComponentRow(m_pages[1], 0, tr("Unlock Icons", "Разблокировка иконок"),
            m_iconsTitle, m_iconsCheck, isUnlockIconsEnabled(),
            [this]() { onToggleIcons(nullptr); });
        addComponentRow(m_pages[1], 1, tr("Unlock Colors", "Открыть все цвета"),
            m_colorsTitle, m_colorsCheck, isUnlockColorsEnabled(),
            [this]() { onToggleColors(nullptr); });
        addComponentRow(m_pages[1], 2, tr("Practice Music", "Музыка в практике"),
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

        // LEVEL
        CCLabelTTF* smartTitle = nullptr;
        CCLabelTTF* smartCheck = nullptr;
        addComponentRow(m_pages[3], 0, tr("Smart Start Pos", "Умные старт-позиции"),
            smartTitle, smartCheck,
            Mod::get()->getSettingValue<bool>("smart-startpos-enabled"),
            [this]() {
                auto mod = Mod::get();
                mod->setSettingValue<bool>("smart-startpos-enabled",
                    !mod->getSettingValue<bool>("smart-startpos-enabled"));
                refreshValues();
            });

        CCLabelTTF* killTitle = nullptr;
        CCLabelTTF* killCheck = nullptr;
        addComponentRow(m_pages[3], 1, tr("Auto Kill At %", "Авто-смерть на %"),
            killTitle, killCheck,
            Mod::get()->getSettingValue<bool>("auto-kill-enabled"),
            [this]() {
                auto mod = Mod::get();
                mod->setSettingValue<bool>("auto-kill-enabled",
                    !mod->getSettingValue<bool>("auto-kill-enabled"));
                refreshValues();
            });

        auto killPercentRow = addPlainRow(m_pages[3], 2,
            tr("Kill percentage", "Процент смерти"), m_speedTitle);
        addValueControls(killPercentRow, m_speedLabel,
            fmt::format("{}%", static_cast<int>(Mod::get()->getSettingValue<float>("auto-kill-percent"))).c_str(),
            [this]() {
                auto mod = Mod::get();
                mod->setSettingValue<float>("auto-kill-percent",
                    std::max(1.f, mod->getSettingValue<float>("auto-kill-percent") - 1.f));
                refreshValues();
            },
            [this]() {
                auto mod = Mod::get();
                mod->setSettingValue<float>("auto-kill-percent",
                    std::min(100.f, mod->getSettingValue<float>("auto-kill-percent") + 1.f));
                refreshValues();
            }, 260.f);

        auto switchRow = addPlainRow(m_pages[3], 3,
            tr("Switch Startpos", "Переключить старт"), m_speedTitle);
        addActionButton(switchRow, tr("Previous", "Назад"), {contentW - 112.f, 14.f},
            76.f, 22.f, []() { gdControlSwitchStartPos(-1); });
        addActionButton(switchRow, tr("Next", "Вперёд"), {contentW - 38.f, 14.f},
            76.f, 22.f, []() { gdControlSwitchStartPos(1); });

        setPage(0);
        refreshValues();
        return true;
    }

    void refreshValues() {
        if (m_tabLabels[0]) m_tabLabels[0]->setString(tr("PLAYER", "ИГРОК"));
        if (m_tabLabels[1]) m_tabLabels[1]->setString(tr("UNLOCKS", "РАЗБЛОКИРОВКИ"));
        if (m_tabLabels[2]) m_tabLabels[2]->setString(tr("BYPASS", "ОБХОДЫ"));
        if (m_tabLabels[3]) m_tabLabels[3]->setString(tr("LEVEL", "УРОВЕНЬ"));

        if (m_noclipTitle) m_noclipTitle->setString(tr("No-Clip", "Ноуклип"));
        if (m_speedTitle) m_speedTitle->setString(tr("Speedhack", "Ускорение"));
        if (m_autoTitle) m_autoTitle->setString(tr("Auto Clicker", "Автокликер"));
        if (m_autoHoldTitle) m_autoHoldTitle->setString(tr("Press", "Нажатие"));
        if (m_autoReleaseTitle) m_autoReleaseTitle->setString(tr("Release", "Отпускание"));
        if (m_iconsTitle) m_iconsTitle->setString(tr("Unlock Icons", "Разблокировка иконок"));
        if (m_colorsTitle) m_colorsTitle->setString(tr("Unlock Colors", "Открыть все цвета"));
        if (m_musicTitle) m_musicTitle->setString(tr("Practice Music", "Музыка в практике"));
        if (m_tpsTitle) m_tpsTitle->setString(tr("TPS Bypass", "Обход TPS"));
        if (m_fpsTitle) m_fpsTitle->setString(tr("FPS Bypass", "Обход FPS"));
        if (m_instantCompleteTitle) m_instantCompleteTitle->setString(
            tr("Instant Complete", "Мгновенное прохождение"));

        if (m_noclipCheck) m_noclipCheck->setString(isNoclipEnabled() ? "✓" : "");
        if (m_iconsCheck) m_iconsCheck->setString(isUnlockIconsEnabled() ? "✓" : "");
        if (m_colorsCheck) m_colorsCheck->setString(isUnlockColorsEnabled() ? "✓" : "");
        if (m_musicCheck) m_musicCheck->setString(isPracticeMusicEnabled() ? "✓" : "");
        if (m_autoCheck) m_autoCheck->setString(isAutoClickerEnabled() ? "✓" : "");
        if (m_tpsCheck) m_tpsCheck->setString(isTPSBypassEnabled() ? "✓" : "");
        if (m_fpsCheck) m_fpsCheck->setString(isFPSBypassEnabled() ? "✓" : "");

        if (m_speedLabel) m_speedLabel->setString(fmt::format("{:.2f}x", getSpeedMultiplier()).c_str());
        if (m_autoHold) m_autoHold->setString(fmt::format("{}", getAutoClickerHold()).c_str());
        if (m_autoRelease) m_autoRelease->setString(fmt::format("{}", getAutoClickerRelease()).c_str());
        if (m_tpsValue) m_tpsValue->setString(fmt::format("{} TPS", getTPSBypassValue()).c_str());
        if (m_fpsValue) m_fpsValue->setString(fmt::format("{} FPS", getFPSBypassValue()).c_str());

    }

    void onInstantComplete(CCObject*) {
        auto layer = PlayLayer::get();
        if (!layer || layer->m_playerDied ||
            layer->m_levelEndAnimationStarted || layer->m_hasCompletedLevel) {
            Notification::create(
                tr("Start a level first", "Сначала запусти уровень"),
                NotificationIcon::Info, 2.f
            )->show();
            return;
        }
        layer->levelComplete();
    }

    void onToggleNoclip(CCObject*) {
        Mod::get()->setSettingValue<bool>("noclip-enabled", !isNoclipEnabled());
        refreshValues();
    }
    void onToggleIcons(CCObject*) {
        Mod::get()->setSettingValue<bool>("unlock-icons", !isUnlockIconsEnabled());
        refreshValues();
    }
    void onToggleColors(CCObject*) {
        Mod::get()->setSettingValue<bool>("unlock-colors", !isUnlockColorsEnabled());
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
    void onExit() override {
        Popup::onExit();
        if (g_activePopup == this) g_activePopup = nullptr;
    }

    static GDControlPopup* create() {
        if (g_activePopup) return nullptr;
        auto ret = new GDControlPopup();
        if (ret && ret->init()) {
            ret->autorelease();
            g_activePopup = ret;
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
        CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(static_cast<cocos2d::CCTargetedTouchDelegate*>(this), -1000, true);
    }

    void onExit() override {
        CCDirector::sharedDirector()->getTouchDispatcher()->removeDelegate(static_cast<cocos2d::CCTargetedTouchDelegate*>(this));
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
        if (!g_activePopup) {
            if (auto popup = GDControlPopup::create()) popup->show();
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
        if (isUnlockIconsEnabled()) return true;
        return GameManager::isIconUnlocked(iconID, type);
    }

    bool isColorUnlocked(int colorID, UnlockType type) {
        if (isUnlockColorsEnabled()) return true;
        return GameManager::isColorUnlocked(colorID, type);
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
    log::info("GD Control v0.2.8 loaded for Geometry Dash 2.2091.");
    installFloatingButton();
    if (isFPSBypassEnabled()) applyFPSBypass();
}
