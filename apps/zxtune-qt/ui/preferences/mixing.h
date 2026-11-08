/**
 *
 * @file
 *
 * @brief Mixing settings pane interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/playlist/supp/data.h"

#include <QtWidgets/QWidget>

class PlaybackSupport;

namespace UI
{
  //! @brief Currently played item as seen by the mixer pages
  struct PlaybackState
  {
    Playlist::Item::Data::Ptr Item;
    bool Paused = false;
  };

  class MixingSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit MixingSettingsWidget(QWidget& parent);

  public:
    enum Kind
    {
      //! AY/YM chips, single and TurboSound
      AYM,
      //! 3- and 4-channel digital (Covox, SounDrive) tunes
      DAC
    };

    //! @param playback source of playback state changes, may be null
    static MixingSettingsWidget* Create(QWidget& parent, Kind kind, PlaybackSupport* playback,
                                        const PlaybackState& state);

    //! @brief Page applicable to the item, if any
    static bool GetKind(const Playlist::Item::Data& item, Kind& kind);
  };
}  // namespace UI
