/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#ifndef niftkIGIDataSourceManager_h
#define niftkIGIDataSourceManager_h

#include "niftkIGIDataSourcesManagerExports.h"
#include <niftkIGIDataSourceFactoryServiceI.h>
#include <niftkIGIDataSourceI.h>
#include <niftkIGIDataType.h>

#include <usServiceReference.h>
#include <usModuleContext.h>

#include <mitkDataStorage.h>
#include <mitkCommon.h>
#include <itkVersion.h>
#include <itkObject.h>
#include <itkObjectFactoryBase.h>
#include <igtlTimeStamp.h>

#include <QMap>
#include <QTimer>
#include <QList>
#include <QString>
#include <QObject>
#include <QMutex>

namespace niftk
{

/**
 * \class IGIDataSourceManager
 * \brief Class to manage a list of IGIDataSources (trackers, ultra-sound machines, video etc).
 *
 * This class should not contain Widget related stuff, so we can instantiate it directly
 * in any class, or a command line app or something without a GUI. It can still
 * derive from QObject, so that we have the benefit of signals and slots.
 *
 * Note: All errors should be thrown as mitk::Exception or sub-class thereof.
 */
class NIFTKIGIDATASOURCESMANAGER_EXPORT IGIDataSourceManager : public QObject
{

  Q_OBJECT

public:

  static const int     DEFAULT_FRAME_RATE;
  static const char*   DEFAULT_RECORDINGDESTINATION_ENVIRONMENTVARIABLE;

  IGIDataSourceManager(mitk::DataStorage::Pointer dataStorage, QObject* parent);
  virtual ~IGIDataSourceManager();

  /**
  * \brief Returns true if the manager is currently playing back, and false otherwise.
  */
  bool IsPlayingBack() const;

  /**
  * \brief Returns true if the manager is playing back automatically, and false otherwise.
  */
  bool IsPlayingBackAutomatically() const;

  /**
  * \brief Returns true if the update timer is on, and false otherwise.
  */
  bool IsUpdateTimerOn() const;

  /**
  * \brief Sets a flag, then the internal clock triggering will also advance the
  * playback time such that the manager will automatically creep forward through time.
  */
  void SetIsPlayingBackAutomatically(bool isPlayingBackAutomatically);

  /**
  * \brief Stops the internal timer.
  */
  void StopUpdateTimer();

  /**
  * \brief Starts the internal timer.
  *
  * If there are no sources, this does nothing.
  */
  void StartUpdateTimer();

  /**
   * \brief Gets a suitable directory name from a prefix determined by preferences, and a date-time stamp.
   */
  QString GetDirectoryName();

  /**
  * \brief Sets the base directory into which all recording sessions will be saved.
  *
  * This is normally set via a GUI preference, so remains unchanged as each
  * recording session is recorded into a new sub-directory within this directory.
  */
  void SetDirectoryPrefix(const QString& directoryPrefix);

  /**
  * \brief Sets the update rate, effectively the number of times
  * per second the internal timer ticks, and the number of times
  * the mitk::RenderingManager is asked to update.
  */
  void SetFramesPerSecond(const int& framesPerSecond);

  /**
  * \brief Returns the frame rate.
  */
  int GetFramesPerSecond() const;

  /**
  * \brief Retrieves the name of all the available data source factory names.
  *
  * The returned list is the display name, as shown in the GUI,
  * e.g. "OpenCV Frame Grabber", and these strings are
  * created in each data sources factory class.
  */
  QList<QString> GetAllFactoryNames() const;

  /**
  * \brief When creating sources, some will need configuring (e.g. port number).
  * So, given the display name of a data source (string in combo-box in GUI),
  * will return true if the manager can create a GUI for you to configure the service.
  */
  bool NeedsStartupGui(QString name);

  /**
  * \brief Adds a source, using the display name of a factory,
  * and configures it with the provided properties.
  */
  void AddSource(QString name, QMap<QString, QVariant>& properties);

  /**
  * \brief Removes a source at a given rowIndex.
  *
  * This will stop the timer, and if, after removing the specified source
  * there are still sources to continue updating, will restart the timer.
  */
  void RemoveSource(int rowIndex);

  /**
  * \brief Removes all sources.
  *
  * This stops the timer, and removes all sources, after which
  * there is no point restarting the timer, so a side effect
  * is that the timer should be off.
  */
  void RemoveAllSources();

  /**
  * \brief Starts a new recording session, writing to the folder given by the absolutePath.
  */
  void StartRecording(QString absolutePath);

  /**
  * \brief Stops the recording process.
  */
  void StopRecording();

  /**
   * \brief Returns true if the data source manager is in recording mode.
   */
  bool IsRecording() const;

  /**
  * \brief Freezes the data sources (i.e. does not do update).
  *
  * Does not affect the saving of data. The data source can
  * continue to grab data, and save it, as it feels like.
  */
  void FreezeAllDataSources(bool isFrozen);

  /**
  * \brief Freezes individual data sources (i.e. does not do update).
  *
  * Does not affect the saving of data. The data source can
  * continue to grab data, and save it, as it feels like.
  */
  void FreezeDataSource(unsigned int i, bool isFrozen);

  /**
  * \brief Returns true if data source i is frozen and false otherwise.
  */
  bool IsFrozen(unsigned int i) const;

  /**
  * \brief Sets the manager ready for playback.
  *
  * \param directoryPrefix path to the root folder of the recording session
  * \param descriptorPath path to a descriptor to parse.
  * \param startTime returns the minimum of start times of all available data sources.
  * \param endTime returns the maximum of end times of all available data sources.
  *
  * Here, the user must have created the right data sources. The reason is
  * that each data-source might need configuring each time you set up
  * the system.  e.g. An Aurora tracker might be connected to a specific COM port.
  * These configurations might be different between record and playback.
  */
  void StartPlayback(const QString& directoryPrefix,
                     const QString& descriptorPath,
                     IGIDataSourceI::IGITimeType& startTime,
                     IGIDataSourceI::IGITimeType& endTime,
                     int& sliderMax,
                     int& sliderSingleStep,
                     int& sliderPageStep,
                     int& sliderValue
                     );

  /**
  * \brief Stops all sources playing back.
  */
  void StopPlayback();

  /**
  * \brief Takes a string time field and computes the corresponding slider value.
  */
  int ComputePlaybackTimeSliderValue(QString textEditField) const;

  /**
  * \brief If the user moves the time slider, we calculate a corresponding time.
  */
  IGIDataSourceI::IGITimeType ComputeTimeFromSlider(int sliderValue) const;

  /**
  * \brief Sets the current time of the manager to time,
  * and the next available update pulse will trigger a refresh.
  */
  void SetPlaybackTime(const IGIDataSourceI::IGITimeType& time);

  /**
  * \brief Requests the manager to grab the currently focussed screen.
  */
  void SetIsGrabbingScreen(QString directoryName, bool isGrabbing);

  /**
  * \brief Retrieves a factory given the display name.
  */
  niftk::IGIDataSourceFactoryServiceI* GetFactory(QString name);

  /**
  * \brief Returns the factory given the row number in the GUI
  * which should correspond to the order of the source in this manager.
  */
  niftk::IGIDataSourceFactoryServiceI* GetFactory(int rowNumber);

  /**
  * \brief Returns the actual source.
  */
  niftk::IGIDataSourceI::Pointer GetSource(int rowNumber);

  /**
   * \brief Performs a global re-init.
   */
  void GlobalReInit();

  /**
   * \brief Returns a reasonable, default, (e.g. Desktop) place to write output.
   */
  static QString GetDefaultWritablePath();

signals:

  /**
  * \brief Emmitted when this manager has asked each data source to update, and they have all updated.
  */
  void UpdateFinishedDataSources(niftk::IGIDataSourceI::IGITimeType, QList< QList<IGIDataItemInfo> >);

  /**
  * \brief Emmitted when this manager has called for rendering to be updated, and that call has completed.
  *
  * (This doesn't mean that the rendering has actually happened. That depends on the mitk::RenderingManager).
  */
  void UpdateFinishedRendering();

  /**
  * \brief When playing back, this class calculates the next time step,
  * and broadcasts what the corresponding slider value should be.
  */
  void PlaybackTimerAdvanced(int sliderValue);

  /**
  * \brief Manager emits the time, to update the GUI.
  */
  void TimerUpdated(QString rawTimeStamp, QString humanReadableTimeStamp);

  /**
  * \brief This manager will broadcast status messages to anyone listening.
  */
  void BroadcastStatusString(QString);

  /**
  * \brief Emmitted when recording has successfully started.
  */
  void RecordingStarted(QString basedirectory);

  /**
  * \brief Emmitted when recording has successfully stopped.
  */
  void RecordingStopped();

protected:

  IGIDataSourceManager(const IGIDataSourceManager&); // Purposefully not implemented.
  IGIDataSourceManager& operator=(const IGIDataSourceManager&); // Purposefully not implemented.

private slots:

  /**
  * \brief Triggered by QTimer, updates the whole rendered scene,
  * based on all the available sources.
  */
  void OnUpdateGui();

private:

  /**
  * \brief Inspects the module registry to retrieve the list of all data source factories.
  */
  void RetrieveAllDataSourceFactories();

  /**
  * Tries to parse the data source descriptor for directory-to-classname mappings.
  * \param filepath full qualified path to descriptor.cfg,
  * e.g. "/home/jo/projectwork/2014-01-28-11-51-04-909/descriptor.cfg"
  * \returns a map with key = directory, value = classname
  * \throws std::exception if something goes wrong.
  * \warning This method does not check whether any class name is valid, i.e. whether that class has been compiled in!
  */
  QMap<QString, QString> ParseDataSourceDescriptor(const QString& filepath);

  /**
  * \brief Internal method to step forward in time, if playing back.
  */
  void AdvancePlaybackTimer();

  /**
  * \brief Internal method to simply set the m_IsPlayingBack field,
  * which should be called from StartPlayback() and StopPlayback().
  */
  void SetIsPlayingBack(bool isPlayingBack);

  /**
  * \brief Writes the descriptor file for a recording session.
  *
  * This descriptor is then used to reconstruct the right number
  * of data sources when you playback.
  */
  void WriteDescriptorFile(QString absolutePath);

  /**
  * \brief A bit of a hack to grab the currently focussed screen, and save as .png.
  */
  void GrabScreen();

  mitk::DataStorage::Pointer                                       m_DataStorage;
  us::ModuleContext*                                               m_ModuleContext;
  std::vector<us::ServiceReference<IGIDataSourceFactoryServiceI> > m_Refs;
  QList<niftk::IGIDataSourceI::Pointer>                            m_Sources;
  QMap<QString, niftk::IGIDataSourceFactoryServiceI*>              m_NameToFactoriesMap;
  QMap<QString, niftk::IGIDataSourceFactoryServiceI*>              m_LegacyNameToFactoriesMap;
  QMutex                                                           m_Lock;
  QTimer                                                          *m_GuiUpdateTimer;
  int                                                              m_FrameRate;
  QString                                                          m_DirectoryPrefix;
  QString                                                          m_PlaybackPrefix;
  igtl::TimeStamp::Pointer                                         m_TimeStampGenerator;
  bool                                                             m_IsPlayingBack;
  bool                                                             m_IsPlayingBackAutomatically;
  niftk::IGIDataSourceI::IGITimeType                               m_CurrentTime;

  int                                                              m_PlaybackSliderValue;
  int                                                              m_PlaybackSliderMaxValue;

  // Slider position is relative to this base value.
  // Slider can only represent int values, but we need all 64 bit.
  niftk::IGIDataSourceI::IGITimeType                               m_PlaybackSliderBase;
  niftk::IGIDataSourceI::IGITimeType                               m_PlaybackSliderFactor;

  bool                                                             m_IsGrabbingScreen;
  QString                                                          m_ScreenGrabDir;

  bool                                                             m_IsRecording;
}; // end class;

} // end namespace

#endif
