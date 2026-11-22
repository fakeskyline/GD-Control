#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>

#include <algorithm>
#include <vector>

using namespace geode::prelude;

namespace {
    bool g_hasSelection = false;
    Ref<StartPosObject> g_selected;
    std::vector<std::pair<Ref<StartPosObject>, bool>> g_overrides;

    void restoreOverrides() {
        for (auto& [object, wasDisabled] : g_overrides) {
            if (object && object->m_startSettings) {
                object->m_startSettings->m_disableStartPos = wasDisabled;
            }
        }
        g_overrides.clear();
    }

    std::vector<StartPosObject*> collectStartPositions(LevelEditorLayer* editor) {
        std::vector<StartPosObject*> positions;
        if (!editor || !editor->m_objects) return positions;

        for (auto object : CCArrayExt<GameObject*>(editor->m_objects)) {
            if (object && object->m_objectID == 31) {
                auto startPos = static_cast<StartPosObject*>(object);
                if (startPos->m_startSettings) positions.push_back(startPos);
            }
        }

        std::stable_sort(positions.begin(), positions.end(),
            [](StartPosObject* a, StartPosObject* b) {
                return a->getPositionX() < b->getPositionX();
            });
        return positions;
    }

    void applySelection(LevelEditorLayer* editor) {
        restoreOverrides();
        if (!g_hasSelection || !editor) return;

        auto positions = collectStartPositions(editor);
        if (g_selected &&
            std::find(positions.begin(), positions.end(), g_selected.data()) == positions.end()) {
            g_hasSelection = false;
            g_selected = nullptr;
            return;
        }

        for (auto startPos : positions) {
            g_overrides.emplace_back(
                startPos, startPos->m_startSettings->m_disableStartPos
            );
            startPos->m_startSettings->m_disableStartPos = startPos != g_selected.data();
        }
    }

    void showNotice(std::string const& message) {
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (!scene) return;

        if (auto old = scene->getChildByID("gd-control-startpos-notice"_spr)) {
            old->removeFromParent();
        }

        auto label = CCLabelBMFont::create(message.c_str(), "bigFont.fnt");
        if (!label) return;
        label->setID("gd-control-startpos-notice"_spr);
        label->setScale(0.45f);
        auto win = CCDirector::sharedDirector()->getWinSize();
        label->setPosition({win.width * 0.5f, win.height - 32.f});
        scene->addChild(label, 1000);
        label->runAction(CCSequence::create(
            CCDelayTime::create(0.9f),
            CCFadeOut::create(0.35f),
            CCRemoveSelf::create(),
            nullptr
        ));
    }

    bool switchStartPos(int direction) {
        auto editor = LevelEditorLayer::get();
        if (!editor || !editor->m_editorUI) return false;

        const bool playing = editor->m_playbackMode != PlaybackMode::Not;
        auto ui = editor->m_editorUI;
        if (!playing && (ui->m_selectedObject ||
            (ui->m_selectedObjects && ui->m_selectedObjects->count() > 0))) {
            return false;
        }

        auto positions = collectStartPositions(editor);
        std::vector<StartPosObject*> slots;
        if (Mod::get()->getSettingValue<bool>("startpos-include-level-start") ||
            positions.empty()) {
            slots.push_back(nullptr);
        }
        slots.insert(slots.end(), positions.begin(), positions.end());

        if (slots.size() < 2) {
            showNotice(positions.empty() ? "No Start Positions" : "Only 1 Start Position");
            return true;
        }

        StartPosObject* current = nullptr;
        if (playing) current = editor->m_startPosObject;
        else if (g_hasSelection) current = g_selected.data();
        else current = editor->findStartPosObject();

        auto found = std::find(slots.begin(), slots.end(), current);
        int index = found == slots.end() ? 0 : static_cast<int>(found - slots.begin());
        const int count = static_cast<int>(slots.size());
        index = ((index + direction) % count + count) % count;
        auto target = slots[index];

        g_hasSelection = true;
        g_selected = target;

        if (playing) {
            ui->onStopPlaytest(nullptr);
            ui->onPlaytest(nullptr);
        } else if (target) {
            ui->centerCameraOnObject(target);
        } else {
            auto win = CCDirector::sharedDirector()->getWinSize();
            auto scale = editor->m_objectLayer->getScale();
            editor->m_objectLayer->setPosition(
                win * 0.5f - CCPoint(0.f, 105.f) * scale
            );
            ui->constrainGameLayerPosition();
        }

        if (target) {
            const int number = static_cast<int>(
                std::find(positions.begin(), positions.end(), target) - positions.begin()
            ) + 1;
            showNotice(fmt::format("Start Pos {}/{}", number, positions.size()));
        } else {
            showNotice("Level Start");
        }
        return true;
    }
}

class $modify(GDControlStartPosEditor, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool noUI) {
        restoreOverrides();
        g_hasSelection = false;
        g_selected = nullptr;
        return LevelEditorLayer::init(level, noUI);
    }

    void onPlaytest() {
        applySelection(this);
        LevelEditorLayer::onPlaytest();
    }

    void onStopPlaytest() {
        LevelEditorLayer::onStopPlaytest();
        restoreOverrides();
    }
};

$on_mod(Loaded) {
    listenForKeybindSettingPresses("startpos-previous-key",
        [](Keybind const&, bool down, bool repeat, double) {
            if (!down || repeat) return false;
            return switchStartPos(-1);
        });
    listenForKeybindSettingPresses("startpos-next-key",
        [](Keybind const&, bool down, bool repeat, double) {
            if (!down || repeat) return false;
            return switchStartPos(1);
        });
}
