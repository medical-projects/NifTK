#/*============================================================================
#
#  NifTK: A software platform for medical image computing.
#
#  Copyright (c) University College London (UCL). All rights reserved.
#
#  This software is distributed WITHOUT ANY WARRANTY; without even
#  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#  PURPOSE.
#
#  See LICENSE.txt in the top level directory for details.
#
#============================================================================*/

####################################################################################
# Set the list of MITK based GUI application names. So, you can add more apps here.
# However, you will also need to specify stuff in each subdirectory.
####################################################################################

mark_as_advanced(${PROJECT_NAME}_INSTALL_RPATH_RELATIVE)

if(MITK_USE_QT)
  set(QT_QMAKE_EXECUTABLE ${MITK_QMAKE_EXECUTABLE})
  add_definitions(-DQWT_DLL)
endif()

####################################################################################
# Add 'Modules', i.e. shared libs
####################################################################################
add_subdirectory(Modules)

#####################################################################################
# Do not remove or rename this.
# It overrides the macro in MITK, and I haven't yet worked out why it actually works.
#####################################################################################
macro(GetMyTargetLibraries all_target_libraries varname)
  set(re_ctkplugin "^(uk_ac_ucl|it_unito_cim)_[a-zA-Z0-9_]+$")
  set(_tmp_list)
  list(APPEND _tmp_list ${all_target_libraries})
  ctkMacroListFilter(_tmp_list re_ctkplugin OUTPUT_VARIABLE ${varname})
endmacro()

####################################################################################
# Create all CTK plugins.
####################################################################################

if(MITK_USE_BLUEBERRY)

  # Don't put this in niftkMacroCreateGuiApplication, because at
  # THIS point specifically, the list of plugins only contains MITK plugins.
  ctkFunctionGetAllPluginTargets(all_mitk_ctk_plugins)

  # Get infos about application directories and build options
  include("${CMAKE_CURRENT_SOURCE_DIR}/Applications/Apps.cmake")
  set(_apps_fullpath )
  foreach(NIFTK_APP ${NIFTK_APPS})
    string(REPLACE "^^" "\\;" target_info ${NIFTK_APP})
    set(target_info_list ${target_info})
    list(GET target_info_list 1 option_name)
    list(GET target_info_list 0 app_name)
    if(${option_name})
      list(APPEND _apps_fullpath "${CMAKE_CURRENT_SOURCE_DIR}/Applications/${NIFTK_APP}")
    endif()
  endforeach()

  # This is to get a list of plugins to pass to ctkMacroSetupPlugins
  include(${CMAKE_CURRENT_SOURCE_DIR}/Plugins/Plugins.cmake)

  list(REMOVE_DUPLICATES PROJECT_PLUGINS)

  # This is required, because people turn the apps off and expect all the
  # corresponding plugins to do the same, unless needed by another app.
  # Note that the argument is a list and therefore it needs to be protected
  # by double quotes so that it is passed to the function as a single argument.

  niftkMacroForcePluginCacheValues("${PROJECT_PLUGINS}")

  ctkMacroSetupPlugins(${PROJECT_PLUGINS}
                       APPS ${_apps_fullpath} 
                       BUILD_OPTION_PREFIX ${PROJECT_NAME}_
                       BUILD_ALL ${${PROJECT_NAME}_BUILD_ALL_PLUGINS})

  set(NIFTK_PLUGIN_USE_FILE "${CMAKE_BINARY_DIR}/NifTKPluginUseFile.cmake")
  if(${PROJECT_NAME}_PLUGIN_LIBRARIES)
    ctkFunctionGeneratePluginUseFile(${NIFTK_PLUGIN_USE_FILE})
  else()
    file(REMOVE ${NIFTK_PLUGIN_USE_FILE})
    set(NIFTK_PLUGIN_USE_FILE )
  endif()

  ctkFunctionGetAllPluginTargets(all_niftk_ctk_plugins)
  GetMyTargetLibraries("${all_niftk_ctk_plugins}" all_niftk_plugins)
  set_property(GLOBAL PROPERTY NIFTK_EXPORTED_PLUGINS ${all_niftk_plugins})

endif(MITK_USE_BLUEBERRY)

####################################################################################
# Temporary Work Around. See issue 4082.
# In MITK 2014.10, the macro mitkFunctionGetLibrarySearchPaths assumes you
# are loading GDCM and ITK from the build folder. As of 4082 we need to load
# these from the installation folder. This may be fixed by MITK around 9th Feb.
# So this can be removed if our version of MITK is post 9th Feb 2015 and the
# packaging still appears to work without this code being present.
#################################################################################### 

if(NOT ITK_DIR OR NOT VTK_DIR OR NOT GDCM_DIR)
  message("Fatal: ITK_DIR, VTK_DIR and GDCM_DIR must be defined")
else()
  get_property(_additional_search_paths GLOBAL PROPERTY MITK_ADDITIONAL_LIBRARY_SEARCH_PATHS)
  foreach(_install_dir ${CMAKE_PREFIX_PATH})
    list(APPEND _additional_search_paths ${_install_dir}/lib)
    list(APPEND _additional_search_paths ${_install_dir}/bin) # On Windows, DLLs go in bin.
  endforeach()
  set_property(GLOBAL PROPERTY MITK_ADDITIONAL_LIBRARY_SEARCH_PATHS ${_additional_search_paths}) 
endif()

####################################################################################
# End of aforementioned Temporary Work Around.
####################################################################################
if (Python_DIR)
  get_property(_additional_search_paths GLOBAL PROPERTY MITK_ADDITIONAL_LIBRARY_SEARCH_PATHS)
  list(APPEND _additional_search_paths ${Python_DIR}/lib)
  list(APPEND _additional_search_paths ${Python_DIR}/bin) # On Windows, DLLs go in bin.
  set_property(GLOBAL PROPERTY MITK_ADDITIONAL_LIBRARY_SEARCH_PATHS ${_additional_search_paths})
endif()


####################################################################################
# Add Applications subdirectory.
#   Applications contains full MITK apps using MITK application framework BlueBerry,
#   as well independent stuff like utilities, command line apps etc.
####################################################################################

add_subdirectory(Applications)
