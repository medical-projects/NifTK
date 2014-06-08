/*=============================================================================

  NifTK: A software platform for medical image computing.

  Copyright (c) University College London (UCL). All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  See LICENSE.txt in the top level directory for details.

=============================================================================*/

#include "QmitkIGINiftyLinkDataSource.h"

//-----------------------------------------------------------------------------
QmitkIGINiftyLinkDataSource::QmitkIGINiftyLinkDataSource(mitk::DataStorage* storage, NiftyLinkSocketObject *socket)
: QmitkIGIDataSource(storage)
, m_Socket(socket)
, m_ClientDescriptor(NULL)
{
  if (m_Socket == NULL)
  {
    m_Socket = new NiftyLinkSocketObject();
    m_UsingSomeoneElsesSocket = false;
  }
  else
  {
    m_UsingSomeoneElsesSocket = true;
  }
  connect(m_Socket, SIGNAL(ClientConnectedSignal()), this, SLOT(ClientConnected()));
  connect(m_Socket, SIGNAL(ClientDisconnectedSignal()), this, SLOT(ClientDisconnected()));
  connect(m_Socket, SIGNAL(MessageReceivedSignal(NiftyLinkMessage::Pointer )), this, SLOT(InterpretMessage(NiftyLinkMessage::Pointer )));
  if (socket != NULL)
  {
  this->ClientConnected();
  }
}


//-----------------------------------------------------------------------------
QmitkIGINiftyLinkDataSource::~QmitkIGINiftyLinkDataSource()
{
  if ( m_UsingSomeoneElsesSocket )
  {
    m_Socket = NULL;
  }
  if (m_Socket != NULL )
  {
    delete m_Socket;
  }
  if (m_ClientDescriptor != NULL)
  {
    delete m_ClientDescriptor;
  }
}


//-----------------------------------------------------------------------------
int QmitkIGINiftyLinkDataSource::GetPort() const
{
  int result = -1;
  if (m_Socket != NULL)
  {
    result = m_Socket->GetPort();
  }
  return result;
}


//-----------------------------------------------------------------------------
void QmitkIGINiftyLinkDataSource::SendMessage(NiftyLinkMessage::Pointer msg)
{
  if (m_Socket != NULL)
  {
    m_Socket->SendMessage(msg);
  }
}


//-----------------------------------------------------------------------------
bool QmitkIGINiftyLinkDataSource::ListenOnPort(int portNumber)
{
  bool isListening = m_Socket->ListenOnPort(portNumber);
  if (isListening)
  {
    this->SetStatus("Listening");
  }
  else
  {
    this->SetStatus("Listening Failed");
  }
  emit DataSourceStatusUpdated(this->GetIdentifier());
  return isListening;
}


//-----------------------------------------------------------------------------
void QmitkIGINiftyLinkDataSource::ClientConnected()
{
  this->SetStatus("Client Connected");
  emit DataSourceStatusUpdated(this->GetIdentifier());
}


//-----------------------------------------------------------------------------
void QmitkIGINiftyLinkDataSource::ClientDisconnected()
{
  this->SetStatus("Listening");
  emit DataSourceStatusUpdated(this->GetIdentifier());
}


//-----------------------------------------------------------------------------
void QmitkIGINiftyLinkDataSource::ProcessClientInfo(ClientDescriptorXMLBuilder* clientInfo)
{
  this->SetClientDescriptor(clientInfo);

  this->SetName(clientInfo->GetDeviceName().toStdString());
  this->SetType(clientInfo->GetDeviceType().toStdString());

  QString descr = QString("Address=") +  clientInfo->GetClientIP()
      + QString(":") + clientInfo->GetClientPort();
  
  // Don't set description for trackers
  if ( clientInfo->GetDeviceType() != "Tracker" ) 
    this->SetDescription(descr.toStdString());

  QString deviceInfo;
  deviceInfo.append("Client connected:");
  deviceInfo.append("  Device name: ");
  deviceInfo.append(clientInfo->GetDeviceName());
  deviceInfo.append("\n");

  deviceInfo.append("  Device type: ");
  deviceInfo.append(clientInfo->GetDeviceType());
  deviceInfo.append("\n");

  deviceInfo.append("  Communication type: ");
  deviceInfo.append(clientInfo->GetCommunicationType());
  deviceInfo.append("\n");

  deviceInfo.append("  Port name: ");
  deviceInfo.append(clientInfo->GetPortName());
  deviceInfo.append("\n");

  deviceInfo.append("  Client ip: ");
  deviceInfo.append(clientInfo->GetClientIP());
  deviceInfo.append("\n");

  deviceInfo.append("  Client port: ");
  deviceInfo.append(clientInfo->GetClientPort());
  deviceInfo.append("\n");

  qDebug() << deviceInfo;
  emit DataSourceStatusUpdated(this->GetIdentifier());
}


//-----------------------------------------------------------------------------
NiftyLinkSocketObject* QmitkIGINiftyLinkDataSource::GetSocket()
{
  return this->m_Socket;
}