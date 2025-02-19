////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Arc Welder: Inverse Processor (firmware simulator).  
// Please see the copyright notices in the function definitions
//
// Converts G2/G3(arc) commands back to G0/G1 commands.  Intended to test firmware changes to improve arc support.
// This reduces file size and the number of gcodes per second.
// 
// Based on arc interpolation implementations from:
//    Marlin 1.x (see https://github.com/MarlinFirmware/Marlin/blob/1.0.x/LICENSE for the current license)
//    Marlin 2.x (see https://github.com/MarlinFirmware/Marlin/blob/2.0.x/LICENSE for the current license)
//    Prusa-Firmware (see https://github.com/prusa3d/Prusa-Firmware/blob/MK3/LICENSE for the current license)
//    Smoothieware (see https://github.com/Smoothieware/Smoothieware for the current license)
//    Repetier (see https://github.com/repetier/Repetier-Firmware for the current license)
// 
// Built using the 'Arc Welder: Anti Stutter' library
//
// Copyright(C) 2021 - Brad Hochgesang
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This program is free software : you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU Affero General Public License for more details.
//
//
// You can contact the author at the following email address: 
// FormerLurker@pm.me
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "firmware_types.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

#define DEFAULT_FIRMWARE_TYPE firmware_types::MARLIN_2
#define LATEST_FIRMWARE_VERSION_NAME "LATEST_RELEASE"
#define DEFAULT_FIRMWARE_VERSION_NAME LATEST_FIRMWARE_VERSION_NAME
// Arc interpretation settings:
#define DEFAULT_MM_PER_ARC_SEGMENT 1.0 // REQUIRED - The enforced maximum length of an arc segment
#define DEFAULT_ARC_SEGMENTS_PER_R 0;
#define DEFAULT_MIN_MM_PER_ARC_SEGMENT 0 /* OPTIONAL - the enforced minimum length of an interpolated segment.  Must be smaller than
  MM_PER_ARC_SEGMENT.  Only has an effect if MIN_ARC_SEGMENTS > 0 or ARC_SEGMENTS_PER_SEC > 0 */
  // If both MIN_ARC_SEGMENTS and ARC_SEGMENTS_PER_SEC is defined, the minimum calculated segment length is used.
#define DEFAULT_MIN_ARC_SEGMENTS 24 // OPTIONAL - The enforced minimum segments in a full circle of the same radius.
#define DEFAULT_MIN_CIRCLE_SEGMENTS 72 // OPTIONAL - The enforced minimum segments in a full circle of the same radius.
#define DEFAULT_ARC_SEGMENTS_PER_SEC 0 // OPTIONAL - Use feedrate to choose segment length.
// approximation will not be used for the first segment.  Subsequent segments will be corrected following DEFAULT_N_ARC_CORRECTION.
#define DEFAULT_N_ARC_CORRECTIONS 24
// This setting is for the gcode position processor to help interpret G90/G91 behavior
#define DEFAULT_G90_G91_INFLUENCES_EXTRUDER false
// This currently is only used in Smoothieware.   The maximum error for line segments that divide arcs.  Set to 0 to disable.
#define DEFAULT_MM_MAX_ARC_ERROR 0.01

struct firmware_state {
  firmware_state() {
    is_relative = false;
    is_extruder_relative = false;
  }
  bool is_relative;
  bool is_extruder_relative;
};

struct firmware_position {
  firmware_position() {
    x = 0;
    y = 0;
    z = 0;
    e = 0;
    f = 0;
  }
  double x;
  double y;
  double z;
  double e;
  double f;
};

struct firmware_arguments {
public:

    firmware_arguments() {
    mm_per_arc_segment = DEFAULT_MM_PER_ARC_SEGMENT;
    arc_segments_per_r = DEFAULT_ARC_SEGMENTS_PER_R;
    min_mm_per_arc_segment = DEFAULT_MIN_MM_PER_ARC_SEGMENT;
    min_arc_segments = DEFAULT_MIN_ARC_SEGMENTS;
    arc_segments_per_sec = DEFAULT_ARC_SEGMENTS_PER_SEC;
    n_arc_correction = DEFAULT_N_ARC_CORRECTIONS;
    g90_g91_influences_extruder = DEFAULT_G90_G91_INFLUENCES_EXTRUDER;
    mm_max_arc_error = DEFAULT_MM_MAX_ARC_ERROR;
    version = DEFAULT_FIRMWARE_VERSION_NAME;
    firmware_type = (firmware_types)DEFAULT_FIRMWARE_TYPE;
    latest_release_version = LATEST_FIRMWARE_VERSION_NAME;

    // add a list of all possible arguments, including aliases
    all_arguments_.clear();
    all_arguments_.push_back("mm_per_arc_segment");
    all_arguments_.push_back("arc_segments_per_r");
    all_arguments_.push_back("min_mm_per_arc_segment");
    all_arguments_.push_back("min_arc_segments");
    all_arguments_.push_back("arc_segments_per_sec");
    all_arguments_.push_back("n_arc_correction");
    all_arguments_.push_back("g90_g91_influences_extruder");
    all_arguments_.push_back("mm_max_arc_error");
    all_arguments_.push_back("min_circle_segments");
    all_arguments_.push_back("min_arc_segment_mm");
    all_arguments_.push_back("max_arc_segment_mm");
  };

  /// <summary>
  /// The maximum mm per arc segment.
  /// </summary>
  double mm_per_arc_segment;
  /// <summary>
  /// The maximum segment length
  /// </summary>
  double arc_segments_per_r;
  /// <summary>
  /// The minimum mm per arc segment.  If less than or equal to 0, this is disabled
  /// </summary>
  double min_mm_per_arc_segment;
  /// <summary>
  /// The number of arc segments that will be drawn per second based on the given feedrate.  
  /// If less than or equal to zero, this is disabled.
  /// </summary>
  double arc_segments_per_sec;
  /// <summary>
  /// This currently is only used in Smoothieware.   The maximum error for line segments that divide arcs.  Set to 0 to disable.
  /// </summary>
  double mm_max_arc_error;
  /// <summary>
  /// The minimum number of arc segments in a full circle of the arc's radius.
  /// If less than or equal to zero, this is disabled
  /// </summary>
  int min_arc_segments;
  /// <summary>
  /// // Number of interpolated segments before true sin and cos corrections will be applied.  
  /// If less than or equal to zero, true sin and cos will always be used.
  /// </summary>
  int n_arc_correction;
  /// <summary>
  /// This value will set the behavior of G90/G91.
  /// </summary>
  bool g90_g91_influences_extruder;

  /// <summary>
  /// The type of firmware to use when interpolating.
  /// </summary>
  firmware_types firmware_type;
  /// <summary>
  /// The firmware version to use.  Defaults to LATEST
  /// </summary>
  std::string version;
  /// <summary>
  /// True if the current version is the latest release.  For informational purposes only
  /// </summary>
  std::string latest_release_version;
  /// Aliases for variour parameters
  int get_min_circle_segments() const
  {
    return min_arc_segments;
  }
  void set_min_circle_segments(int segments)
  {
    min_arc_segments = segments;
  }

  double get_min_arc_segment_mm() const
  {
    return min_mm_per_arc_segment;
  }

  void set_min_arc_segment_mm(double mm)
  {
    min_mm_per_arc_segment = mm;
  }

  double get_max_arc_segment_mm() const
  {
    return mm_per_arc_segment;
  }

  void set_max_arc_segment_mm(double mm)
  {
    mm_per_arc_segment = mm;
  }

  void set_used_arguments(std::vector<std::string> arguments)
  {
    used_arguments_ = arguments;
  }

  std::vector<std::string> get_unused_arguments()
  {
    std::vector<std::string> unused_arguments;
    for (std::vector<std::string>::iterator it = all_arguments_.begin(); it != all_arguments_.end(); it++)
    {
      if (!is_argument_used(*it))
      {
        unused_arguments.push_back(*it);
      }
    }
    return unused_arguments;
  }

  std::string get_unused_arguments_string()
  {
    std::string unusaed_argument_string = "";
    std::vector<std::string> unused_argumnts = get_unused_arguments();
    for (std::vector<std::string>::iterator it = unused_argumnts.begin(); it != unused_argumnts.end(); it++)
    {
      if (unusaed_argument_string.size() > 0)
      {
        unusaed_argument_string += ", ";
      }
      unusaed_argument_string += *it;
    }
    return unusaed_argument_string;
  }

  std::vector<std::string> get_available_arguments()
  {
    return used_arguments_;
  }

  std::string get_argument_description() {
    std::stringstream stream;
    stream << "Firmware Arguments:\n";
    stream << "\tFirmware Type               : " << firmware_type_names[firmware_type] << "\n";
    stream << "\tFirmware Version            : " << (version == LATEST_FIRMWARE_VERSION_NAME || version == latest_release_version ? latest_release_version + " (" + LATEST_FIRMWARE_VERSION_NAME + ")" : version) <<"\n";
    stream << std::fixed << std::setprecision(0);
    // Bool values
    if (is_argument_used("g90_g91_influences_extruder"))
    {
      stream << "\tg90_g91_influences_extruder : " << (g90_g91_influences_extruder ? "True" : "False") << "\n";
    }
    
    // Int values
    if (is_argument_used("min_arc_segments"))
    {
      stream << "\tmin_arc_segments            : " << min_arc_segments << "\n";
    }
    if (is_argument_used("min_circle_segments"))
    {
      stream << "\tmin_circle_segments         : " << get_min_circle_segments() << "\n";
    }
    if (is_argument_used("n_arc_correction"))
    {
      stream << "\tn_arc_correction            : " << n_arc_correction << "\n";
    }

    stream << std::fixed << std::setprecision(2);
    // Double values
    // 
    if (is_argument_used("mm_per_arc_segment"))
    {
      stream << "\tmm_per_arc_segment          : " << mm_per_arc_segment << "\n";
    }
    // 
    if (is_argument_used("arc_segments_per_r"))
    {
      stream << "\tarc_segments_per_r          : " << arc_segments_per_r << "\n";
    }
    // 
    if (is_argument_used("min_mm_per_arc_segment"))
    {
      stream << "\tmin_mm_per_arc_segment      : " << min_mm_per_arc_segment << "\n";
    }
    // 
    if (is_argument_used("arc_segments_per_sec"))
    {
      stream << "\tarc_segments_per_sec        : " << arc_segments_per_sec << "\n";
    }
    // 
    if (is_argument_used("mm_max_arc_error"))
    {
      stream << "\tmm_max_arc_error            : " << mm_max_arc_error << "\n";
    }
    // 
    if (is_argument_used("min_arc_segment_mm"))
    {
      stream << "\tmin_arc_segment_mm          : " << get_min_arc_segment_mm() << "\n";
    }
    // 
    if (is_argument_used("max_arc_segment_mm"))
    {
      stream << "\tmax_arc_segment_mm          : " << get_max_arc_segment_mm() << "\n";
    }

    std::string unused_argument_string = get_unused_arguments_string();
    if (unused_argument_string.size() > 0)
    {
      stream << "The following parameters do not apply to this firmware version: " << unused_argument_string << "\n";
    }
    return stream.str();

  }
  bool is_argument_used(std::string argument_name)
  {
    return (std::find(used_arguments_.begin(), used_arguments_.end(), argument_name) != used_arguments_.end());
  }
  private:
    std::vector<std::string> all_arguments_;
    std::vector<std::string> used_arguments_;
    
};

class firmware
{
public:

  firmware();

  firmware(firmware_arguments args);

  /// <summary>
  /// Generate G1 gcode strings separated by line breaks representing the supplied G2/G3 command.
  /// </summary>
  /// <param name="current">The current printer position</param>
  /// <param name="target">The target printer position</param>
  /// <param name="i">Specifies the X offset for the arc's center.</param>
  /// <param name="j">Specifies the Y offset for the arc's center.</param>
  /// <param name="r">Specifies the radius of the arc.  If r is greater than 0, this will override the i and j parameters.</param>
  /// <param name="is_clockwise">If true, this is a G2 command.  If false, this is a G3 command.</param>
  /// <param name="is_relative">If this is true, the extruder is currently in relative mode.  Else it is in absolute mode.</param>
  /// <param name="offest_absolute_e">This is the absolute offset for absolute E coordinates if the extruder is not in relative mode.</param>
  /// <returns></returns>
  virtual std::string interpolate_arc(firmware_position& target, double i, double j, double r, bool is_clockwise);

  /// <summary>
  /// Sets the current position.  Should be called before interpolate_arc.
  /// </summary>
  /// <param name="position">The position to set</param>
  void set_current_position(firmware_position& position);

  /// <summary>
  /// Sets firmware offsets and the xyze axis mode.
  /// </summary>
  /// <param name="state">The state to set</param>
  void set_current_state(firmware_state& state);
  /// <summary>
  /// Create a G1 command from the current position and offsets.
  /// </summary>
  /// <param name="target">The position of the printer after the G1 command is completed.</param>
  /// <returns>The G1 command</returns>
  virtual std::string g1_command(firmware_position& target);

  /// <summary>
  /// Checks a string to see if it is a valid version.
  /// </summary>
  /// <param name="version">The version to check.</param>
  /// <returns>True if the supplied version is valid</returns>
  bool is_valid_version(std::string version);

  /// <summary>
  /// Returns all valid versions for this firmware.
  /// </summary>
  /// <returns>Vector of strings, one for each supported version</returns>
  std::vector<std::string> get_version_names();

  /// <summary>
  /// Returns the current g90_g91_influences_extruder value for the firmware.
  /// </summary>
  /// <returns></returns>
  bool get_g90_g91_influences_extruder();

  /// <summary>
  /// Returns the number of arc segments that were generated from g2/g3 commands.
  /// </summary>
  /// <returns></returns>
  int get_num_arc_segments_generated();

  /// <summary>
  /// Outputs a string description of the firmware arguments.
  /// </summary>
  /// <returns></returns>
  std::string get_argument_description();

  /// <summary>
  /// Sets all available versions names and the version index based on args_.version
  /// </summary>
  /// <returns></returns>
  void set_versions(std::vector<std::string> version_names, std::string latest_release_version_name);

  virtual firmware_arguments get_default_arguments_for_current_version()const;


  void set_arguments(firmware_arguments args);

  virtual void apply_arguments();

protected:
  firmware_position position_;
  firmware_state state_;
  firmware_arguments args_;
  std::vector<std::string> version_names_;
  int version_index_;
  int num_arc_segments_generated_;

  virtual firmware_arguments arguments_changed(firmware_arguments current_args, firmware_arguments new_args);
};
