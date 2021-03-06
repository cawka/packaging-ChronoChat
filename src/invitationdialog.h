/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef INVITATIONDIALOG_H
#define INVITATIONDIALOG_H

#include <QDialog>

#ifndef Q_MOC_RUN
#include <ndn.cxx/data.h>
#include <ndn.cxx/security/certificate/identity-certificate.h>
#include "chronos-invitation.h"
#endif

namespace Ui {
class InvitationDialog;
}

class InvitationDialog : public QDialog
{
    Q_OBJECT

public:
  explicit InvitationDialog(QWidget *parent = 0);
  ~InvitationDialog();

  void
  setInvitation(const std::string& alias,
                ndn::Ptr<ChronosInvitation> invitation, 
                ndn::Ptr<ndn::security::IdentityCertificate> identityCertificate);

signals:
  void
  invitationAccepted(const ChronosInvitation& invitation, 
                     const ndn::security::IdentityCertificate& identityCertificate);
  
  void
  invitationRejected(const ChronosInvitation& invitation);

private slots:
  void
  onOkClicked();
  
  void
  onCancelClicked();


private:
  Ui::InvitationDialog *ui;
  std::string m_inviterAlias;
  ndn::Ptr<ChronosInvitation> m_invitation;
  ndn::Ptr<ndn::security::IdentityCertificate> m_identityCertificate;
};

#endif // INVITATIONDIALOG_H
