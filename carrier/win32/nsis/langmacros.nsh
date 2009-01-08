;;
;; Windows Pidgin NSIS installer language macros
;;

!macro PIDGIN_MACRO_DEFAULT_STRING LABEL VALUE
  !ifndef "${LABEL}"
    !define "${LABEL}" "${VALUE}"
    !ifdef INSERT_DEFAULT
      !warning "${LANG} lang file missing ${LABEL}, using default..."
    !endif
  !endif
!macroend

!macro PIDGIN_MACRO_LANGSTRING_INSERT LABEL LANG
  LangString "${LABEL}" "${LANG_${LANG}}" "${${LABEL}}"
  !undef "${LABEL}"
!macroend

!macro PIDGIN_MACRO_LANGUAGEFILE_BEGIN LANG
  !define CUR_LANG "${LANG}"
!macroend

!macro PIDGIN_MACRO_LANGUAGEFILE_END
  !define INSERT_DEFAULT
  !include "${PIDGIN_DEFAULT_LANGFILE}"
  !undef INSERT_DEFAULT

  ; Pidgin Language file Version 3
  ; String labels should match those from the default language file.

  ; Startup checks
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT INSTALLER_IS_RUNNING		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_IS_RUNNING			${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT GTK_INSTALLER_NEEDED		${CUR_LANG}

  ; License Page
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_LICENSE_BUTTON			${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_LICENSE_BOTTOM_TEXT		${CUR_LANG}

  ; Components Page
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SECTION_TITLE			${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT GTK_SECTION_TITLE			${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SHORTCUTS_SECTION_TITLE	${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_DESKTOP_SHORTCUT_SECTION_TITLE ${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_STARTMENU_SHORTCUT_SECTION_TITLE	${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SECTION_DESCRIPTION		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT GTK_SECTION_DESCRIPTION		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SHORTCUTS_SECTION_DESCRIPTION	${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_DESKTOP_SHORTCUT_DESC		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_STARTMENU_SHORTCUT_DESC	${CUR_LANG}

  ; GTK+ Directory Page
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT GTK_UPGRADE_PROMPT			${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT GTK_WINDOWS_INCOMPATIBLE		${CUR_LANG}

  ; Installer Finish Page
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_FINISH_VISIT_WEB_SITE		${CUR_LANG}

  ; Pidgin Section Prompts and Texts
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_PROMPT_CONTINUE_WITHOUT_UNINSTALL	${CUR_LANG}

  ; GTK+ Section Prompts
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT GTK_INSTALL_ERROR			${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT GTK_BAD_INSTALL_PATH		${CUR_LANG}

  ; URI Handler section
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT URI_HANDLERS_SECTION_TITLE		${CUR_LANG}

  ; Uninstall Section Prompts
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT un.PIDGIN_UNINSTALL_ERROR_1		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT un.PIDGIN_UNINSTALL_ERROR_2		${CUR_LANG}

  ; Spellcheck Section Prompts
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_SECTION_TITLE	${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_ERROR		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_DICT_ERROR		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_SECTION_DESCRIPTION	${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT ASPELL_INSTALL_FAILED		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_BRETON		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_CATALAN		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_CZECH		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_WELSH		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_DANISH		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_GERMAN		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_ENGLISH		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_GREEK		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_ESPERANTO		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_SPANISH		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_FAROESE		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_FRENCH		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_ITALIAN		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_DUTCH		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_NORWEGIAN		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_POLISH		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_PORTUGUESE		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_ROMANIAN		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_RUSSIAN		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_SLOVAK		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_SWEDISH		${CUR_LANG}
  !insertmacro PIDGIN_MACRO_LANGSTRING_INSERT PIDGIN_SPELLCHECK_UKRAINIAN		${CUR_LANG}

  !undef CUR_LANG
!macroend

!macro PIDGIN_MACRO_INCLUDE_LANGFILE LANG FILE
  !insertmacro PIDGIN_MACRO_LANGUAGEFILE_BEGIN "${LANG}"
  !include "${FILE}"
  !insertmacro PIDGIN_MACRO_LANGUAGEFILE_END
!macroend