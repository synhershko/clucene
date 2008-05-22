dnl AX_CONFIG_FEATURE
AC_DEFUN([AX_CONFIG_FEATURE],[ dnl
m4_pushdef([FEATURE], patsubst([$1], -, _))dnl

AC_ARG_ENABLE([$1],AC_HELP_STRING([--enable-$1],[$2]),[
case "${enableval}" in
   yes)
     ax_config_feature_[]FEATURE[]="yes"
     ;;
   no)
     ax_config_feature_[]FEATURE[]="no"
     ;;
   *)
     AC_MSG_ERROR([bad value ${enableval} for feature --$1])
     ;;
esac
])

AS_IF([test "$ax_config_feature_[]FEATURE[]" = yes],[ dnl
  AC_DEFINE([$3])
  $5
  AS_IF([test "$ax_config_feature_verbose" = yes],[ dnl
    AC_MSG_NOTICE([Feature $1 is enabled])
  ])
],[ dnl
  $6
  AS_IF([test "$ax_config_feature_verbose" = yes],[ dnl
    AC_MSG_NOTICE([Feature $1 is disabled])
  ])
])

AH_TEMPLATE([$3],[$4])

m4_popdef([FEATURE])dnl
])

dnl Feature global
AC_DEFUN([AX_CONFIG_FEATURE_VERBOSE],[ dnl
  ax_config_feature_verbose=yes
])

dnl Feature global
AC_DEFUN([AX_CONFIG_FEATURE_SILENT],[ dnl
  ax_config_feature_verbose=no
])

dnl Feature specific
AC_DEFUN([AX_CONFIG_FEATURE_DEFAULT_ENABLED], [
  ax_config_feature_[]FEATURE[]_default=yes
])

dnl Feature specific
AC_DEFUN([AX_CONFIG_FEATURE_DEFAULT_DISABLED], [
  ax_config_feature_[]FEATURE[]_default=no
])

dnl Feature specific
AC_DEFUN([AX_CONFIG_FEATURE_ENABLE],[ dnl
  ax_config_feature_[]patsubst([$1], -, _)[]=yes
])

dnl Feature specific
AC_DEFUN([AX_CONFIG_FEATURE_DISABLE],[ dnl
  ax_config_feature_[]patsubst([$1], -, _)[]=yes
])
