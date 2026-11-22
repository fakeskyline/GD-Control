#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <algorithm>
#include <vector>

using namespace geode::prelude;

namespace {
    bool g_autoKillTriggered = false;

    bool smartStartPosEnabled() {
        return Mod::get()->getSettingValue<bool>("smart-startpos-enabled");
    }

    bool autoKillEnabled() {
        return Mod::get()->getSettingValue<bool>("auto-kill-enabled");
    }

    float autoKillPercent() {
        return std::clamp(Mod::get()->getSettingValue<float>("auto-kill-percent"), 1.f, 100.f);
    }

    GameObject* closestBefore(std::vector<GameObject*> const& objects, StartPosObject* startPos) {
        GameObject* closest = nullptr;
        for (auto object : objects) {
            if (!object || object->getPositionX() >= startPos->getPositionX() - 10.f) continue;
            if (!closest || object->getPositionX() > closest->getPositionX()) closest = object;
        }
        return closest;
    }

    void configureStartPos(
        StartPosObject* startPos, LevelSettingsObject* levelSettings,
        std::vector<GameObject*> const& dual, std::vector<GameObject*> const& modes,
        std::vector<GameObject*> const& mini, std::vector<GameObject*> const& speeds
    ) {
        if (!startPos || !startPos->m_startSettings || !levelSettings) return;
        auto settings = startPos->m_startSettings;
        settings->m_startDual = levelSettings->m_startDual;
        settings->m_startMode = levelSettings->m_startMode;
        settings->m_startMini = levelSettings->m_startMini;
        settings->m_startSpeed = levelSettings->m_startSpeed;

        if (auto object = closestBefore(dual, startPos))
            settings->m_startDual = object->m_objectID == 286;

        if (auto object = closestBefore(modes, startPos)) {
            switch (object->m_objectID) {
                case 12: settings->m_startMode = 0; break;
                case 13: settings->m_startMode = 1; break;
                case 47: settings->m_startMode = 2; break;
                case 111: settings->m_startMode = 3; break;
                case 660: settings->m_startMode = 4; break;
                case 745: settings->m_startMode = 5; break;
                case 1331: settings->m_startMode = 6; break;
                case 1933: settings->m_startMode = 7; break;
                default: break;
            }
        }

        if (auto object = closestBefore(mini, startPos))
            settings->m_startMini = object->m_objectID == 101;

        if (auto object = closestBefore(speeds, startPos)) {
            switch (object->m_objectID) {
                case 200: settings->m_startSpeed = Speed::Slow; break;
                case 201: settings->m_startSpeed = Speed::Normal; break;
                case 202: settings->m_startSpeed = Speed::Fast; break;
                case 203: settings->m_startSpeed = Speed::Faster; break;
                case 1334: settings->m_startSpeed = Speed::Fastest; break;
                default: break;
            }
        }
    }
}

class $modify(GDControlSmartStartPos, PlayLayer) {
    struct Fields {
        std::vector<StartPosObject*> startPositions;
        std::vector<GameObject*> dualPortals;
        std::vector<GameObject*> modePortals;
        std::vector<GameObject*> miniPortals;
        std::vector<GameObject*> speedPortals;
    };

    bool init(GJGameLevel* level, bool dontCreateObjects) {
        m_fields->startPositions.clear();
        m_fields->dualPortals.clear();
        m_fields->modePortals.clear();
        m_fields->miniPortals.clear();
        m_fields->speedPortals.clear();
        g_autoKillTriggered = false;
        return PlayLayer::init(level, dontCreateObjects);
    }

    void addObject(GameObject* object) {
        PlayLayer::addObject(object);
        if (!object) return;
        switch (object->m_objectID) {
            case 31: m_fields->startPositions.push_back(static_cast<StartPosObject*>(object)); break;
            case 286: case 287: m_fields->dualPortals.push_back(object); break;
            case 12: case 13: case 47: case 111: case 660: case 745: case 1331: case 1933:
                m_fields->modePortals.push_back(object); break;
            case 99: case 101: m_fields->miniPortals.push_back(object); break;
            case 200: case 201: case 202: case 203: case 1334:
                m_fields->speedPortals.push_back(object); break;
            default: break;
        }
    }

    void resetLevel() {
        g_autoKillTriggered = false;
        if (smartStartPosEnabled() && m_levelSettings) {
            for (auto startPos : m_fields->startPositions) {
                configureStartPos(startPos, m_levelSettings, m_fields->dualPortals,
                    m_fields->modePortals, m_fields->miniPortals, m_fields->speedPortals);
            }
        }
        PlayLayer::resetLevel();
    }
};

class $modify(GDControlAutoKill, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        auto layer = PlayLayer::get();
        if (!autoKillEnabled() || !layer || g_autoKillTriggered ||
            layer->m_playerDied || layer->m_hasCompletedLevel ||
            layer->m_levelEndAnimationStarted) return;
        if (layer->getCurrentPercent() < autoKillPercent()) return;

        g_autoKillTriggered = true;
        const bool noclipWasEnabled = Mod::get()->getSettingValue<bool>("noclip-enabled");
        if (noclipWasEnabled) Mod::get()->setSettingValue<bool>("noclip-enabled", false);
        if (layer->m_player1 && !layer->m_player1->m_isDead)
            layer->PlayLayer::destroyPlayer(layer->m_player1, layer->m_player1);
        if (layer->m_player2 && !layer->m_player2->m_isDead)
            layer->PlayLayer::destroyPlayer(layer->m_player2, layer->m_player2);
        if (noclipWasEnabled) Mod::get()->setSettingValue<bool>("noclip-enabled", true);
    }
};
