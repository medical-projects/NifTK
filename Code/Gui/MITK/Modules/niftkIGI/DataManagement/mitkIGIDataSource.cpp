/*=============================================================================

 NifTK: An image processing toolkit jointly developed by the
             Dementia Research Centre, and the Centre For Medical Image Computing
             at University College London.

 See:        http://dementia.ion.ucl.ac.uk/
             http://cmic.cs.ucl.ac.uk/
             http://www.ucl.ac.uk/

 Last Changed      : $Date: 2012-07-25 07:31:59 +0100 (Wed, 25 Jul 2012) $
 Revision          : $Revision: 9401 $
 Last modified by  : $Author: mjc $

 Original author   : m.clarkson@ucl.ac.uk

 Copyright (c) UCL : See LICENSE.txt in the top level directory for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 ============================================================================*/

#include "mitkIGIDataSource.h"
#include <itkObjectFactory.h>
#include <igtlTimeStamp.h>

namespace mitk
{

//-----------------------------------------------------------------------------
IGIDataSource::IGIDataSource()
: m_DataStorage(NULL)
, m_Identifier(-1)
, m_FrameRate(0)
, m_CurrentFrameId(0)
, m_Name("")
, m_Type("")
, m_Status("")
, m_Description("")
, m_SavingMessages(false)
, m_SaveOnReceipt(true)
, m_SaveInBackground(false)
, m_SavePrefix("")
, m_RequestedTimeStamp(0)
, m_ActualTimeStamp(0)
, m_TimeStampTolerance(1000000000)
, m_ActualData(NULL)
{
  m_RequestedTimeStamp = igtl::TimeStamp::New();
  m_RequestedTimeStamp->GetTime();

  m_ActualTimeStamp = igtl::TimeStamp::New();
  m_ActualTimeStamp->GetTime();

  m_Buffer.clear();
  m_BufferIterator = m_Buffer.begin();
  m_FrameRateBufferIterator = m_Buffer.begin();
}


//-----------------------------------------------------------------------------
IGIDataSource::~IGIDataSource()
{
  // We don't own the m_DataStorage, so don't delete it.
}

//-----------------------------------------------------------------------------
void IGIDataSource::SetSavingMessages(bool isSaving)
{
  this->m_SavingMessages = isSaving;
  this->Modified();

  SaveStateChanged.Send();
}


//-----------------------------------------------------------------------------
igtlUint64 IGIDataSource::GetFirstTimeStamp() const
{
  igtlUint64 timeStamp = 0;

  if (m_Buffer.size() > 0)
  {
    timeStamp = m_Buffer.front()->GetTimeStampInNanoSeconds();
  }

  return timeStamp;
}


//-----------------------------------------------------------------------------
igtlUint64 IGIDataSource::GetLastTimeStamp() const
{
  igtlUint64 timeStamp = 0;

  if (m_Buffer.size() > 0)
  {
    timeStamp = m_Buffer.back()->GetTimeStampInNanoSeconds();
  }

  return timeStamp;
}


//-----------------------------------------------------------------------------
unsigned long int IGIDataSource::GetBufferSize() const
{
  return m_Buffer.size();
}


//-----------------------------------------------------------------------------
void IGIDataSource::ClearBuffer()
{
  unsigned long int bufferSizeBefore = m_Buffer.size();

  m_Buffer.clear();

  unsigned long int bufferSizeAfter = m_Buffer.size();
  MITK_INFO << this->GetName() << ": Clear operation reduced the buffer size from " << bufferSizeBefore << ", to " << bufferSizeAfter << std::endl;

}


//-----------------------------------------------------------------------------
void IGIDataSource::CleanBuffer()
{
  unsigned int approxDoubleTheFrameRate = 1;
  if (this->GetFrameRate() > 0)
  {
    approxDoubleTheFrameRate = (int)(this->GetFrameRate() * 2);
  }

  if (m_Buffer.size() > approxDoubleTheFrameRate)
  {
    unsigned long int bufferSizeBefore = m_Buffer.size();

    std::list<mitk::IGIDataType::Pointer>::iterator iter = m_Buffer.begin();

    while(   m_Buffer.size() > approxDoubleTheFrameRate
          && (*iter).IsNotNull()
          && (!((*iter)->GetShouldBeSaved()) || ((*iter)->GetShouldBeSaved() && (*iter)->GetIsSaved()))
          && ((*iter)->GetTimeStampInNanoSeconds() < GetTimeInNanoSeconds(this->m_ActualTimeStamp))
        )
    {
      m_Buffer.erase(iter);
      iter++;
    }

    unsigned long int bufferSizeAfter = m_Buffer.size();
    MITK_INFO << this->GetName() << ": Clean operation reduced the buffer size from " << bufferSizeBefore << ", to " << bufferSizeAfter << std::endl;
  }
}


//-----------------------------------------------------------------------------
mitk::IGIDataType* IGIDataSource::RequestData(igtlUint64 requestedTimeStamp)
{
  // Aim here is to iterate through the buffer, and find the closest
  // message to the requested time stamp, and leave the m_BufferIterator,
  // m_ActualTimeStamp and m_ActualData at that point, and return the corresponding data.

  SetTimeInNanoSeconds(m_RequestedTimeStamp, requestedTimeStamp);

  if (m_Buffer.size() == 0)
  {
    SetTimeInNanoSeconds(m_ActualTimeStamp, 0);
    m_ActualData = NULL;
  }
  else
  {
    if (m_Buffer.size() == 1)
    {
      m_BufferIterator = m_Buffer.begin();
    }
    else
    {
      while(     m_BufferIterator != m_Buffer.end()
            && (*m_BufferIterator).IsNotNull()
            && (*m_BufferIterator)->GetTimeStampInNanoSeconds() < GetTimeInNanoSeconds(m_RequestedTimeStamp)
           )
      {
        m_BufferIterator++;
      }

      if (m_BufferIterator == m_Buffer.end())
      {
        m_BufferIterator--;
      }
      else if (m_BufferIterator != m_Buffer.begin())
      {
        igtlUint64 afterTimeStamp = (*m_BufferIterator)->GetTimeStampInNanoSeconds();

        m_BufferIterator--;

        igtlUint64 beforeTimeStamp = (*m_BufferIterator)->GetTimeStampInNanoSeconds();
        igtlUint64 requestedTimeStamp = GetTimeInNanoSeconds(m_RequestedTimeStamp);

        igtlUint64 beforeToRequested = requestedTimeStamp - beforeTimeStamp;
        igtlUint64 afterToRequested = afterTimeStamp - requestedTimeStamp;

        if (afterToRequested < beforeToRequested)
        {
          m_BufferIterator++;
        }
      }
    }

    m_ActualData = (*m_BufferIterator);
    SetTimeInNanoSeconds(m_ActualTimeStamp, m_ActualData->GetTimeStampInNanoSeconds());
  }

  return m_ActualData;
}


//-----------------------------------------------------------------------------
bool IGIDataSource::IsCurrentWithinTimeTolerance() const
{
  bool result = false;

  igtlUint64 requestedTimeStamp = GetTimeInNanoSeconds(m_RequestedTimeStamp);
  igtlUint64 actualTimeStamp = GetTimeInNanoSeconds(m_ActualTimeStamp);

  if (   m_ActualData != NULL
      && fabs((double)requestedTimeStamp - (double)actualTimeStamp) < m_TimeStampTolerance        // the data source can decide what to accept
      && fabs((double)requestedTimeStamp - (double)actualTimeStamp) < m_ActualData->GetDuration() // the data can have a duration that it is valid for
      )
  {
    result = true;
  }

  return result;
}


//-----------------------------------------------------------------------------
double IGIDataSource::GetCurrentTimeLag()
{
  igtl::TimeStamp::Pointer timeStamp = igtl::TimeStamp::New();
  timeStamp->GetTime();

  double lag = 0;
  igtlUint64 nowTime = GetTimeInNanoSeconds(timeStamp);

  if (m_ActualData != NULL)
  {
    igtlUint64 dataTime = m_ActualData->GetTimeStampInNanoSeconds();
    lag = (double)nowTime - (double)dataTime;
  }
  lag /= 1000000000.0;
  return lag;
}


//-----------------------------------------------------------------------------
void IGIDataSource::UpdateFrameRate()
{
  // default position is zero.
  m_FrameRate = 0;

  // Always initialise...
  igtlUint64 lastTimeStamp = 0;
  unsigned long int lastFrameId = 0;
  igtlUint64 currentTimeStamp = 0;
  unsigned long int currentFrameId = 0;
  igtlUint64 timeDifference = 0;
  unsigned long int numberOfFrames = 0;

  std::list<mitk::IGIDataType::Pointer>::iterator iter = m_Buffer.end();
  iter--;

  if (m_Buffer.size() >= 2 && iter != m_Buffer.begin() && iter != m_Buffer.end() && iter != m_FrameRateBufferIterator)
  {
    lastTimeStamp = (*m_FrameRateBufferIterator)->GetTimeStampInNanoSeconds();
    lastFrameId = (*m_FrameRateBufferIterator)->GetFrameId(); // assumed to be sequentially increasing

    currentTimeStamp = (*iter)->GetTimeStampInNanoSeconds();
    currentFrameId = (*iter)->GetFrameId(); // assumed to be sequentially increasing

    if (currentTimeStamp > lastTimeStamp)
    {
      timeDifference = currentTimeStamp - lastTimeStamp;
      numberOfFrames = currentFrameId - lastFrameId;

      double rate = (double)1.0 / ((double)timeDifference/(double)(numberOfFrames * 1000000000.0));

      m_FrameRateBufferIterator = iter;
      m_FrameRate = rate;
    }
  }
}


//-----------------------------------------------------------------------------
bool IGIDataSource::DoSaveData(mitk::IGIDataType* data)
{
  bool result = false;

  std::string fileName = "";
  if (this->SaveData(data, fileName))
  {
    data->SetIsSaved(true);
    data->SetFileName(fileName);

    result = true;
  }

  return result;
}


//-----------------------------------------------------------------------------
bool IGIDataSource::AddData(mitk::IGIDataType* data)
{
  bool result = false;

  if (data == NULL)
  {
    MITK_ERROR << "IGIDataSource::AddData is receiving NULL data. This is not allowed!" << std::endl;
    return false;
  }

  if (this->CanHandleData(data))
  {
    data->SetShouldBeSaved(this->GetSavingMessages());
    data->SetIsSaved(false);
    data->SetFrameId(m_CurrentFrameId++);

    m_Buffer.push_back(data);

    if (m_Buffer.size() == 1)
    {
      m_BufferIterator = m_Buffer.begin();
      m_FrameRateBufferIterator = m_BufferIterator;
    }


    if (   m_SavingMessages     // recording/saving is turned on.
        && !m_SaveInBackground  // we are doing it immediately as opposed to some background thread.
        && m_SaveOnReceipt      // we are saving every message that came in, regardless of display refresh rate.
        )
    {
      result = this->DoSaveData(data);
    }
    else
    {
      result = true;
    }
  }
  return result;
}


//-----------------------------------------------------------------------------
bool IGIDataSource::ProcessData(igtlUint64 requestedTimeStamp)
{
  bool result = false;
  mitk::IGIDataType* data = this->RequestData(requestedTimeStamp);

  if (data != NULL)
  {
    if (this->IsCurrentWithinTimeTolerance())
    {
      try
      {
        // Decide if we are saving data
        if (   m_SavingMessages    // recording/saving is turned on.
            && !m_SaveInBackground // we are doing it immediately as opposed to some background thread.
            && !m_SaveOnReceipt    // we only save the data that was shown on the display.
            )
        {
          this->DoSaveData(data);
        }

        // Derived classes implement this to update the display.
        result = this->Update(data);

      } catch (mitk::Exception& e)
      {
        MITK_ERROR << "IGIDataSource::AddData: Source=" << this->GetIdentifier() \
                   << ", received error:\nMITK Exception:\n\nDescription: " << e.GetDescription() << "\n\n" \
                   << "Filename: " << e.GetFile() << "\n\n" \
                   << "Line: " << e.GetLine() << std::endl;
      }
      catch (std::exception& e)
      {
        MITK_ERROR << "IGIDataSource::AddData: Source=" << this->GetIdentifier() \
                   << ", received error:" << e.what() << std::endl;
      }
      catch (...)
      {
        MITK_ERROR << "IGIDataSource::AddData: Source=" << this->GetIdentifier() \
                   << ", received error:Unknown exception" << std::endl;
      }
    }
    else
    {
      MITK_DEBUG << "IGIDataSource::AddData: Source=" << this->GetIdentifier() \
                 << ", req=" << requestedTimeStamp \
                 << ", msg=" << data->GetFrameId() \
                 << ", ts=" << data->GetTimeStampInNanoSeconds() \
                 << ", dur=" << data->GetDuration()
                 << ", result=out of date" << std::endl;
    }
  }
  else
  {
    MITK_DEBUG << "IGIDataSource::ProcessData did not process data at requestedTimeStamp=" << requestedTimeStamp << ", as the data was NULL" << std::endl;
  }

  return result;
}

} // end namespace
