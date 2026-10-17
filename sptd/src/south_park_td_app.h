// South Park: Let's Go Tower Defense Play! - ReXGlue Recompiled Project
//
// The app: GPU/audio setup, the TRG launcher before the runtime starts, port
// defaults, achievement art and notifications.

#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>

#include <imgui.h>

#include <rex/audio/sdl/sdl_audio_system.h>
#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/rex_app.h>
#include <rex/runtime.h>
#include <rex/system/achievement_manager.h>
#include <rex/system/xam/user_profile.h>
#include <rex/system/kernel_state.h>
#include <rex/system/util/xdbf_utils.h>
#include <rex/system/xcontent.h>
#include <rex/system/xmemory.h>
#include <rex/system/interfaces/graphics.h>
#include <rex/ui/immediate_drawer.h>
#include <rex/ui/presenter.h>
#include <rex/ui/window.h>
#include <rex/ui/windowed_app_context.h>

#include "trg/game_helpers.h"

#include "art.h"
#include "frame_stats.h"
#include "launcher.h"
#include "overlay.h"
#include "platform.h"
#include "settings.h"
#include "toast.h"
#include "updater.h"

namespace sptd {
void InstallFpeGuard();                             // fpe_guard.cpp
void InstallNullTrace(const uint8_t* guest_base);  // null_trace.cpp
}

class SouthParkTdApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;
  ~SouthParkTdApp() override {
    if (window()) window()->RemoveInputListener(&fullscreen_keys_);
  }

  static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<SouthParkTdApp>(new SouthParkTdApp(ctx, "south_park_td", PPCImageConfig));
  }

 protected:
  void OnConfigurePaths(rex::PathConfig& paths) override {
    // Register the GPU plugin's cvars before the config is read so the
    // launcher can edit and save them, and set the port's own defaults.
    sptd::update::CleanUpPreviousUpdate();  // *.old files from the last update
    sptd::PreloadGpuPlugin();
    sptd::ApplyPortDefaults();
    // An AppImage runs from a read-only mount, so keep the game files and the
    // settings beside the .AppImage file instead of beside the program.
    std::filesystem::path base = rex::filesystem::GetExecutableFolder();
    if (const char* appimage = std::getenv("APPIMAGE"); appimage && *appimage) {
      const auto exe_dir = base;
      base = std::filesystem::path(appimage).parent_path();
      if (paths.config_path.empty() || paths.config_path.parent_path() == exe_dir)
        paths.config_path = base / (paths.config_path.empty() ? std::filesystem::path("south_park_td.toml")
                                                              : paths.config_path.filename());
      if (REXCVAR_GET(log_file).empty()) {
        std::error_code ec;
        std::filesystem::create_directories(base / "logs", ec);
        sptd::SetCvarDefault("log_file", (base / "logs" / "south_park_td.log").string());
      }
    }
    if (paths.game_data_root.empty()) paths.game_data_root = base / "game";
    // Achievement names and icons, written out of the player's default.xex on first run.
    if (paths.metadata_root.empty()) paths.metadata_root = sptd::art::AchievementDir(paths.game_data_root);
  }

  std::optional<rex::PathConfig> OnFinalizePaths(const rex::PathConfig& defaults,
                                                 std::function<void(rex::PathConfig)> resume) override {
    user_data_root_ = defaults.user_data_root;
    sptd_config_path_ = defaults.config_path;
    const bool skip_once = REXCVAR_GET(sptd_skip_launcher);
    rex::cvar::ResetToDefault("sptd_skip_launcher");  // never persist it
    const bool files_ok =
        sptd::GameFilesPresent(defaults.game_data_root) && sptd::GameVersionMatches(defaults.game_data_root);
    const bool show = !files_ok || sptd::IsShiftHeld() || (REXCVAR_GET(sptd_launcher) && !skip_once);
    if (!show) {
      sptd::ApplyRenderPreset(OutputSize().second);
      return defaults;
    }

    sptd::LauncherCallbacks cb;
    cb.play = [this, resume, defaults] {
      app_context().CallInUIThreadDeferred([this, resume, defaults] {
        sptd::ApplyRenderPreset(OutputSize().second);
        resume(defaults);
      });
    };
    cb.restart_and_play = [this] {
      sptd::RelaunchSelf(L"--sptd_skip_launcher=true");
      app_context().CallInUIThreadDeferred([this] { app_context().QuitFromUIThread(); });
    };
    cb.quit = [this] { app_context().CallInUIThreadDeferred([this] { app_context().QuitFromUIThread(); }); };
    cb.set_fullscreen = [this](bool on) {
      if (window()) window()->SetFullscreen(on);
    };
    cb.dpi_scale = [this] { return window() ? double(window()->GetDpi()) / window()->GetMediumDpi() : 1.0; };
    cb.screen_size = [] { return sptd::PrimaryScreenSize(); };
    cb.output_size = [this] { return OutputSize(); };
    sptd::ShowLauncher(imgui_drawer(), immediate_drawer(),
                      {defaults.game_data_root, defaults.user_data_root, defaults.config_path}, std::move(cb));
    return std::nullopt;
  }

  void OnConfigureFonts(ImFontAtlas* atlas) override { sptd::LoadUiFont(atlas); }

  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (!config.graphics && config.gpu_plugin.empty()) config.gpu_plugin = "xenos";
    if (!config.audio_factory) config.audio_factory = REX_AUDIO_BACKEND(rex::audio::sdl::SDLAudioSystem);
  }

  void OnPostSetup() override {
    sptd::InstallFpeGuard();  // after the runtime's own signal handlers
    sptd::InstallNullTrace(runtime()->virtual_membase());
    REXLOG_INFO("SPTD: {}", trg::TuneProcessScheduling());
    {
      const std::filesystem::path log = REXCVAR_GET(log_file);
      const auto logs = log.has_parent_path() ? log.parent_path() : rex::filesystem::GetExecutableFolder() / "logs";
      trg::InstallCrashReports(logs.string(), "South Park Tower Defense");
    }
    sptd::ApplyRuntimeOverrides();

    // F11 / Alt+Enter switch between fullscreen and windowed in game (issue #4).
    if (window()) window()->AddInputListener(&fullscreen_keys_, 1);

    ExportAchievementArt();
    // Give the launcher the achievement names (read from the game by the runtime).
    if (!user_data_root_.empty())
      sptd::art::WriteAchievementCache(achievements().ListAchievements(),
                                      sptd::art::AchievementCachePath(user_data_root_));
    ScheduleTitleCapture();
    ScheduleWelcomeAchievement();

    // Debug aid: set SPTD_DUMP_IMAGE=<file> to write the decrypted guest image
    // for offline analysis (tools/ppc_dis.py, tools/find_*.py).
    const char* dump_path = std::getenv("SPTD_DUMP_IMAGE");
    if (!dump_path || !*dump_path) return;
    const uint8_t* base = runtime()->virtual_membase();
    std::ofstream out(dump_path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(base + PPCImageConfig.image_base), PPCImageConfig.image_size);
  }

  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    SetGuestFrameStats(sptd::GetGuestFrameStats);  // F3 overlay "Guest: N FPS"
    sptd::CreateFpsOverlay(drawer);
  }

  // Xbox 360-style toast with a chime for the game's achievements.
  std::unique_ptr<rex::ui::AchievementNotificationDialog> CreateAchievementNotificationDialog() override {
    auto toast = std::make_unique<sptd::AchievementToast>(imgui_drawer(), immediate_drawer(), game_data_root(),
                                                         user_data_root_);
    toast_ = toast.get();
    return toast;
  }

 private:
  // F11 or Alt+Enter: toggle fullscreen. Setting the cvar makes ReXGlue switch
  // the window; the choice is saved so the launcher and the next start keep it.
  class FullscreenKeys final : public rex::ui::WindowInputListener {
   public:
    explicit FullscreenKeys(SouthParkTdApp& app) : app_(app) {}
    void OnKeyDown(rex::ui::KeyEvent& e) override {
      const auto key = e.virtual_key();
      const bool hotkey = (key == rex::ui::VirtualKey::kF11 && !e.is_alt_pressed()) ||
                          (key == rex::ui::VirtualKey::kReturn && e.is_alt_pressed());
      if (!hotkey || e.prev_state()) return;  // ignore auto-repeat while held
      e.set_handled(true);
      const bool on = !REXCVAR_QUERY(bool, fullscreen);
      rex::cvar::SetFlagByName("fullscreen", on ? "true" : "false");
      if (!app_.sptd_config_path_.empty()) sptd::SaveSettings(app_.sptd_config_path_);
      REXLOG_INFO("SPTD: {} (hotkey)", on ? "fullscreen" : "windowed");
    }

   private:
    SouthParkTdApp& app_;
  };
  FullscreenKeys fullscreen_keys_{*this};
  std::filesystem::path sptd_config_path_;  // the launcher's settings file

  // Size the game is shown at: the monitor in fullscreen, else the window.
  std::pair<int, int> OutputSize() const {
    if (REXCVAR_QUERY(bool, fullscreen) || !window()) return sptd::PrimaryScreenSize();
    return {int(window()->GetActualPhysicalWidth()), int(window()->GetActualPhysicalHeight())};
  }

  // The launcher shows the achievements' names and pictures from
  // game/achievements. On the first run after installing, write them out of
  // the game's own executable (its XDBF resource).
  void ExportAchievementArt() {
    const auto dir = sptd::art::AchievementDir(game_data_root());
    std::error_code ec;
    if (std::filesystem::exists(dir / "achievements.toml", ec)) return;
    auto* ks = runtime() ? runtime()->kernel_state() : nullptr;
    if (!ks) return;
    const auto db = ks->title_xdbf();
    if (!db.is_valid()) return;
    std::filesystem::create_directories(dir / "icons", ec);
    const auto lang = db.GetExistingLanguage(rex::system::XLanguage::kEnglish);
    auto quote = [](const std::string& s) {
      std::string out = "\"";
      for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        if (static_cast<unsigned char>(c) >= 0x20) out += c;
      }
      return out + "\"";
    };
    std::ofstream toml(dir / "achievements.toml", std::ios::binary);
    for (const auto& a : db.GetAchievements()) {
      const uint32_t image = a.image_id;
      toml << "[[achievements]]\n"
           << "id = " << uint32_t(a.id) << "\n"
           << "label = " << quote(db.GetStringTableEntry(lang, a.label_id)) << "\n"
           << "description = " << quote(db.GetStringTableEntry(lang, a.description_id)) << "\n"
           << "unachieved_description = " << quote(db.GetStringTableEntry(lang, a.unachieved_id)) << "\n"
           << "gamerscore = " << uint32_t(a.gamerscore) << "\n"
           << "image_id = " << image << "\n"
           << "icon_path = \"icons/" << image << ".png\"\n\n";
      const auto block = db.GetEntry(rex::system::util::XdbfSection::kImage, image);
      if (block.buffer && block.size) {
        std::ofstream png(dir / "icons" / (std::to_string(image) + ".png"), std::ios::binary);
        png.write(reinterpret_cast<const char*>(block.buffer), std::streamsize(block.size));
      }
    }
    REXLOG_INFO("SPTD: wrote achievement list and icons to {}", dir.string());
  }

  // On first play, keep a frame of the game's title screen as launcher art.
  void ScheduleTitleCapture() {
    if (user_data_root_.empty()) return;
    const auto path = sptd::art::TitleCapturePath(user_data_root_);
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) return;
    sptd::RunAfterFirstFrame(kTitleCaptureDelay, [this, path] {
      app_context().CallInUIThread([this, path] {
        rex::ui::RawImage image;
        auto* gfx = runtime() ? runtime()->graphics_system() : nullptr;
        auto* presenter = gfx ? gfx->presenter() : nullptr;
        if (presenter && presenter->CaptureGuestOutput(image) && sptd::art::SaveTitleCapture(image, path))
          REXLOG_INFO("SPTD: saved launcher art {}x{}", image.width, image.height);
      });
    });
  }

  // The port's own "welcome" achievement: unlocks a few seconds into the first
  // play so players learn the game has achievements.
  void ScheduleWelcomeAchievement() {
    const auto& welcome = sptd::PortAchievements().front();
    if (user_data_root_.empty() || sptd::IsPortAchievementUnlocked(user_data_root_, welcome.id)) return;
    sptd::RunAfterFirstFrame(6.0, [this, &welcome] {
      if (sptd::UnlockPortAchievement(user_data_root_, welcome.id) && toast_) toast_->Show(welcome.title, 0, 0);
    });
  }

  // Seconds after the first frame when the title screen is showing.
  static constexpr double kTitleCaptureDelay = 20.0;

  std::filesystem::path user_data_root_;
  sptd::AchievementToast* toast_ = nullptr;  // owned by ReXApp
};
