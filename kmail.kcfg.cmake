<?xml version="1.0" encoding="UTF-8"?>
<kcfg xmlns="http://www.kde.org/standards/kcfg/1.0"
      xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:schemaLocation="http://www.kde.org/standards/kcfg/1.0
      http://www.kde.org/standards/kcfg/1.0/kcfg.xsd" >
  <include>qtextcodec.h</include>
  <include>kglobalsettings.h</include>
  <include>kcolorscheme.h</include>
  <include>composer.h</include>
  <kcfgfile name="kmailrc"/>
  <group name="Behaviour">
      <entry name="ActionEnterFolder"  type="Enum">
        <label></label>
        <whatsthis></whatsthis>
        <choices>
          <choice name="SelectFirstNew"/>
          <choice name="SelectFirstUnreadNew"/>
          <choice name="SelectLastSelected"/>
          <choice name="SelectNewest"/>
          <choice name="SelectOldest"/>
        </choices>
        <default>SelectLastSelected</default>
      </entry>
      <entry name="NetworkState"  type="Enum">
        <choices>
          <choice name="Online"/>
          <choice name="Offline"/>
        </choices>
        <default>Online</default>
      </entry>
      <entry name="LoopOnGotoUnread"  type="Enum">
        <label></label>
        <whatsthis></whatsthis>
        <choices>
          <choice name="DontLoop"/>
          <choice name="LoopInCurrentFolder"/>
          <choice name="LoopInAllFolders"/>
          <choice name="LoopInAllMarkedFolders"/>
        </choices>
        <default>DontLoop</default>
      </entry>
      <entry name="ShowPopupAfterDnD"  type="Bool">
        <label></label>
        <whatsthis></whatsthis>
        <default>true</default>
      </entry>
      <entry name="ExcludeImportantMailFromExpiry"  type="Bool">
        <label></label>
        <whatsthis>This prevents the automatic expiry of old messages in a folder from deleting (or moving to an archive folder) the messages that are marked 'Important' or 'Action Item'</whatsthis>
        <default>true</default>
      </entry>

      <entry name="SendOnCheck"  type="Enum">
        <label>Send queued mail on mail check</label>
        <whatsthis>&lt;qt&gt;&lt;p&gt;Select whether you want KMail to send all messages in the outbox on manual or all mail checks, or whether you do not want messages to be sent automatically at all. &lt;/p&gt;&lt;/qt&gt;</whatsthis>
        <choices>
          <choice name="DontSendOnCheck"/>
          <choice name="SendOnManualChecks"/>
          <choice name="SendOnAllChecks"/>
        </choices>
        <default>DontSendOnCheck</default>
      </entry>

      <entry name="AutoLostFoundMove" type="Bool">
        <label>Automatically move non-synced mails from folders with insufficient access rights</label>
        <whatsthis>If there are new messages in a folder, which have not been uploaded to the server yet, but you do not have sufficient access rights on the folder now to upload them, these messages will automatically be moved into a lost and found folder.</whatsthis>
        <default>false</default>
      </entry>

      <entry name="AllowLocalFlags" type="Bool">
        <label>Allow local flags in read-only folders</label>
        <default>false</default>
      </entry>
    </group>

    <group name="ConfigurationDialogRestrictions">
      <entry name="MinimumCheckInterval" type="Int">
        <default>1</default>
        <whatsthis>This setting allows administrators to set a minimum delay between two mail checks. The user will not be able to choose a value smaller than the value set here.</whatsthis>
      </entry>
    </group>

    <group name="FolderSelectionDialog">
      <entry name="LastSelectedFolder" type="String">
        <whatsthis>The most recently selected folder in the folder selection dialog.</whatsthis>
        <default>inbox</default>
      </entry>

    </group>

    <group name="General">
      <entry name="WarnBeforeExpire" type="bool" key="warn-before-expire">
        <default>true</default>
      </entry>
      <entry  name="DefaultDomain" type="String" key="Default domain">
        <default></default>
      </entry>
      <entry name="SystemTrayEnabled" type="Bool">
        <label>Enable system tray icon</label>
        <default>false</default>
      </entry>
      <entry name="SystemTrayPolicy" type="Enum">
        <label>Policy for showing the system tray icon</label>
        <choices>
          <choice name="ShowAlways"/>
          <choice name="ShowOnUnread"/>
        </choices>
        <default>ShowOnUnread</default>
      </entry>
      <entry name="CloseDespiteSystemTray" type="Bool">
          <label>Close the application when the main window is closed, even if there is a system tray icon active.</label>
        <default>false</default>
      </entry>
      <entry name="VerboseNewMailNotification" type="Bool">
        <label>Verbose new mail notification</label>
        <whatsthis>If this option is enabled then for each folder the number of newly arrived messages is shown in the new mail notification; otherwise, you will only get a simple 'New mail arrived' message.</whatsthis>
        <default>true</default>
      </entry>
      <entry name="ExternalEditor" type="String" key="external-editor">
        <label>Specify e&amp;ditor:</label>
        <default>kwrite %f</default>
      </entry>
      <entry name="UseExternalEditor" type="Bool" key="use-external-editor">
        <label>Use e&amp;xternal editor instead of composer</label>
        <default>false</default>
      </entry>
      <entry name="CustHeaderCount" type="Int" key="mime-header-count" />
      <entry name="FolderLoadingTimeout" type="Int" hidden="true">
         <default>1000</default>
      </entry>
      <entry name="CloseToQuotaThreshold" type="Int">
        <label>The threshold for when to warn the user that a folder is nearing its quota limit.</label>
         <default>80</default>
      </entry>
    </group>
<!-- General -->


    <group name="Internal">
      <entry name="MsgDictSizeHint" type="Int" hidden="true">
        <default>9973</default>
      </entry>
      <entry name="PreviousNewFeaturesMD5" type="String" hidden="true">
        <whatsthis>This value is used to decide whether the KMail Introduction should be displayed.</whatsthis>
        <default></default>
      </entry>
    </group>

    <group name="Network">
      <entry name="MaxConnectionsPerHost" type="Int">
        <label>Maximal number of connections per host</label>
        <whatsthis>This can be used to restrict the number of connections per host while checking for new mail. By default the number of connections is unlimited (0).</whatsthis>
        <default>0</default>
        <min>0</min>
      </entry>
    </group>

    <group name="UserInterface">
      <entry name="EnableFolderQuickSearch" type="Bool">
        <label>Show folder quick search line edit</label>
        <default>false</default>
      </entry>
      <entry name="HideLocalInbox" type="Bool">
        <label>Hide local inbox if unused</label>
        <default>true</default>
      </entry>
    </group>

    <group name="Composer">
      <entry name="ForwardingInlineByDefault" type="Bool">
        <default>false</default>
        <label>Forward Inline As Default.</label>
      </entry>
      <entry name="StickyIdentity" type="Bool" key="sticky-identity">
        <whatsthis>Remember this identity, so that it will be used in future composer windows as well.
        </whatsthis>
        <default>false</default>
      </entry>
      <entry name="StickyFcc" type="Bool" key="sticky-fcc">
        <whatsthis>Remember this folder for sent items, so that it will be used in future composer windows as well.</whatsthis>
        <default>false</default>
      </entry>
      <entry name="StickyTransport" type="Bool" key="sticky-transport">
        <whatsthis>Remember this mail transport, so that it will be used in future composer windows as well.</whatsthis>
        <default>false</default>
      </entry>
      <entry name="StickyDictionary" type="Bool">
        <whatsthis>Remember this dictionary, so that it will be used in future composer windows as well.
        </whatsthis>
        <default>false</default>
      </entry>
      <entry name="WordWrap" type="Bool" key="word-wrap">
        <label>Word &amp;wrap at column:</label>
        <default>true</default>
      </entry>
      <entry name="LineWrapWidth" type="Int" key="break-at">
        <label></label>
        <default>78</default>
        <min>30</min>
        <max>998</max>
      </entry>
       <entry name="TooManyRecipients" type="Bool" key="too-many-recipients">
        <label>Warn if the number of recipients is larger than</label>
        <default>${WARN_TOOMANY_RECIPIENTS_DEFAULT}</default>
        <whatsthis>If the number of recipients is larger than this value, KMail will warn and ask for a confirmation before sending the mail. The warning can be turned off.</whatsthis>
      </entry>
      <entry name="RecipientThreshold" type="Int" key="recipient-threshold">
        <label></label>
        <default>5</default>
        <min>1</min>
        <max>100</max>
        <whatsthis>If the number of recipients is larger than this value, KMail will warn and ask for a confirmation before sending the mail. The warning can be turned off.</whatsthis>
     </entry>
     <entry name="PreviousIdentity" type="UInt" key="previous-identity" />
      <entry name="PreviousFcc" type="String" key="previous-fcc" />
      <entry name="PreviousDictionary" type="String" />
      <entry name="CurrentTransport" type="String" key="current-transport" />
      <entry name="OutlookCompatibleAttachments" type="Bool" key="outlook-compatible-attachments">
        <label>Outlook-compatible attachment naming</label>
        <whatsthis>Turn this option on to make Outlook &#8482; understand attachment names containing non-English characters</whatsthis>
        <default>false</default>
      </entry>
      <entry name="UseHtmlMarkup" type="Bool" key="html-markup">
        <default>false</default>
      </entry>
      <entry name="PgpAutoSign" type="Bool" key="pgp-auto-sign">
        <default>false</default>
      </entry>
      <entry name="PgpAutoEncrypt" type="Bool" key="pgp-auto-encrypt">
        <default>false</default>
      </entry>
      <entry name="NeverEncryptDrafts" type="Bool" key="never-encrypt-drafts">
        <default>true</default>
      </entry>

      <entry name="ChiasmusKey" type="String" key="chiasmus-key">
      </entry>
      <entry name="ChiasmusOptions" type="String" key="chiasmus-options">
      </entry>

      <entry name="ConfirmBeforeSend" type="Bool" key="confirm-before-send">
        <label>Confirm &amp;before send</label>
        <default>false</default>
      </entry>
      <entry name="RequestMDN" type="Bool" key="request-mdn">
        <label>Automatically request &amp;message disposition notifications</label>
        <whatsthis>&lt;qt&gt;&lt;p&gt;Enable this option if you want KMail to request Message Disposition Notifications (MDNs) for each of your outgoing messages.&lt;/p&gt;&lt;p&gt;This option only affects the default; you can still enable or disable MDN requesting on a per-message basis in the composer, menu item &lt;em&gt;Options&lt;/em&gt;-&gt;&lt;em&gt;Request Disposition Notification&lt;/em&gt;.&lt;/p&gt;&lt;/qt&gt;</whatsthis>
        <default>false</default>
      </entry>
      <entry name="Headers" type="Int" key="headers">
        <default>KMail::Composer::HDR_SUBJECT</default>
      </entry>
      <entry name="CompletionMode" type="Int" key="Completion Mode">
        <default code="true">KGlobalSettings::completionMode()</default>
      </entry>
      <entry name="AutoSpellChecking" type="Bool" key="autoSpellChecking">
        <default>true</default>
      </entry>
      <entry name="ShowForgottenAttachmentWarning" type="Bool" key="showForgottenAttachmentWarning">
        <default>true</default>
      </entry>
      <entry name="AttachmentKeywords" type="StringList" key="attachment-keywords">
        <default code="true">
          i18nc(
                "comma-separated list of keywords that are used to detect whether "
                "the user forgot to attach his attachment",
                "attachment,attached" ).split( ',' )
        </default>
      </entry>
      <entry name="ShowMessagePartDialogOnAttach" type="Bool" key="showMessagePartDialogOnAttach">
        <default>false</default>
      </entry>
      <entry name="AutosaveInterval" type="Int" key="autosave">
        <label>Autosave interval:</label>
        <whatsthis>A backup copy of the text in the composer window can be created regularly. The interval used to create the backups is set here. You can disable autosaving by setting it to the value 0.</whatsthis>
        <default>2</default>
      </entry>
      <entry name="QuoteSelectionOnly" type="Bool">
        <label>Only quote selected text when replying</label>
        <default>true</default>
      </entry>

      <entry name="CustomTemplates" type="StringList" key="CustomTemplates" />

      <entry name="MimetypesToStripWhenInlineForwarding" type="StringList">
          <label>List of message part types to strip off mails that are being forwarded inline.</label>
      </entry>

      <entry name="ShowSnippetManager" type="Bool">
          <label>Show the Text Snippet Management and Insertion Panel in the composer.</label>
          <default>false</default>
      </entry>
      <entry name="SnippetSplitterPosition" type="IntList"/>

      <entry name="MaximumAttachmentSize" type="Int">
          <label>The maximum size in MB that email attachments are allowed to have (-1 for no limit).</label>
          <default>-1</default>
      </entry>

      <entry name="ShowGnuPGAuditLogAfterSuccessfulSignEncrypt" type="Bool">
          <label>Show the GnuPG Audit Log even after crypto operations that completed successfully.</label>
          <default>false</default>
      </entry>

    </group>
<!-- Composer -->

    <group name="Fonts">
      <entry name="ComposerFont" type="Font" key="composer-font">
        <default code="true">KGlobalSettings::generalFont()</default>
      </entry>
    </group>

    <group name="Geometry">
      <entry name="ComposerSize" type="Size" key="composer">
        <default>QSize(480,510)</default>
      </entry>
      <entry name="FolderViewWidth" type="Int">
        <default>250</default>
      </entry>
      <entry name="FolderViewHeight" type="Int">
        <default>500</default>
      </entry>
      <entry name="FolderTreeHeight" type="Int">
        <default>400</default>
      </entry>
      <entry name="SearchAndHeaderHeight" type="Int">
        <default>180</default>
      </entry>
      <entry name="SearchAndHeaderWidth" type="Int">
        <default>450</default>
      </entry>
      <entry name="ReaderWindowWidth" type="Int">
        <default>200</default>
      </entry>
      <entry name="ReaderWindowHeight" type="Int">
        <default>320</default>
      </entry>

      <entry name="readerWindowMode" type="Enum">
       <label>Message Preview Pane</label>
       <choices>
         <choice name="hide">
           <label>Do not show a message preview pane</label>
         </choice>
         <choice name="below">
           <label>Show the message preview pane below the message list</label>
         </choice>
         <choice name="right">
           <label>Show the message preview pane next to the message list</label>
         </choice>
       </choices>
       <default>below</default>
      </entry>

      <entry name="FolderList" type="Enum">
       <label>Folder List</label>
       <choices>
         <choice name="longlist">
           <label>Long folder list</label>
         </choice>
         <choice name="shortlist">
           <label>Short folder list</label>
         </choice>
       </choices>
       <default>longlist</default>
      </entry>
    </group>

    <group name="Reader">
     <entry name="CloseAfterReplyOrForward" type="Bool">
       <label>Close message window after replying or forwarding</label>
       <default>false</default>
     </entry>
    </group>

    <group name="MDN">
      <entry name="SendMDNsWithEmptySender" type="Bool">
        <default>false</default>
        <label>Send Message Disposition Notifications with an empty sender.</label>
        <whatsthis>Send Message Disposition Notifications with an empty sender string. Some servers might be configure to reject such messages, so if you are experiencing problems sending MDNs, uncheck this option.</whatsthis>
      </entry>
    </group>

  <group name="GlobalTemplates">
    <entry name="TemplateNewMessage" type="String" key="TemplateNewMessage">
      <label>Message template for new message</label>
      <whatsthis></whatsthis>
      <default code="true">TemplatesConfiguration::defaultNewMessage()</default>
    </entry>
    <entry name="TemplateReply" type="String" key="TemplateReply">
      <label>Message template for reply</label>
      <whatsthis></whatsthis>
      <default code="true">TemplatesConfiguration::defaultReply()</default>
    </entry>
    <entry name="TemplateReplyAll" type="String" key="TemplateReplyAll">
      <label>Message template for reply to all</label>
      <whatsthis></whatsthis>
      <default code="true">TemplatesConfiguration::defaultReplyAll()</default>
    </entry>
    <entry name="TemplateForward" type="String" key="TemplateForward">
      <label>Message template for forward</label>
      <whatsthis></whatsthis>
      <default code="true">TemplatesConfiguration::defaultForward()</default>
    </entry>
    <entry name="QuoteString" type="String" key="QuoteString">
      <label>Quote characters</label>
      <whatsthis></whatsthis>
      <default code="true">TemplatesConfiguration::defaultQuoteString()</default>
    </entry>
  </group>

    <group name="OutOfOffice">
        <entry name="AllowOutOfOfficeSettings" type="Bool">
            <default>true</default>
            <label>Allow out-of-office settings to be changeable by the user.</label>
        </entry>
        <entry name="AllowOutOfOfficeUploadButNoSettings" type="Bool">
            <default>false</default>
            <label>Allow users to upload out-of-office sieve scripts, but prevent them from changing any settings, such as the domain to react to or the spam reaction switch.</label>
        </entry>
        <entry name="OutOfOfficeDomain" type="String">
            <default></default>
            <label>Send out-of-office replies to mails coming from this domain only.</label>
        </entry>
        <entry name="OutOfOfficeReactToSpam" type="Bool">
            <default>false</default>
            <label>Allow out-of-office replies to be sent to messages marked as SPAM.</label>
        </entry>
        <entry name="CheckOutOfOfficeOnStartup" type="Bool">
              <default>true</default>
              <label>Check if there is still an active out-of-office reply configured when starting KMail.</label>
        </entry>
    </group>

  <group name="FavoriteFolderView">
    <entry name="EnableFavoriteFolderView" type="Bool">
      <default>true</default>
    </entry>
    <entry name="FavoriteFolderViewHeight" type="Int">
      <default>100</default>
    </entry>
    <entry name="FavoriteFolderIds" type="IntList"/>
    <entry name="FavoriteFolderNames" type="StringList"/>
    <entry name="FavoriteFolderViewSeenInboxes" type="IntList"/>
  </group>

</kcfg>
