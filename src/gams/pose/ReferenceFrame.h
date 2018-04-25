/**
 * Copyright (c) 2015 Carnegie Mellon University. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following acknowledgments and disclaimers.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. The names "Carnegie Mellon University," "SEI" and/or "Software
 *    Engineering Institute" shall not be used to endorse or promote products
 *    derived from this software without prior written permission. For written
 *    permission, please contact permission@sei.cmu.edu.
 * 
 * 4. Products derived from this software may not be called "SEI" nor may "SEI"
 *    appear in their names without prior written permission of
 *    permission@sei.cmu.edu.
 * 
 * 5. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 * 
 *      This material is based upon work funded and supported by the Department
 *      of Defense under Contract No. FA8721-05-C-0003 with Carnegie Mellon
 *      University for the operation of the Software Engineering Institute, a
 *      federally funded research and development center. Any opinions,
 *      findings and conclusions or recommendations expressed in this material
 *      are those of the author(s) and do not necessarily reflect the views of
 *      the United States Department of Defense.
 * 
 *      NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
 *      INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
 *      UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
 *      IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
 *      FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
 *      OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES
 *      NOT MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
 *      TRADEMARK, OR COPYRIGHT INFRINGEMENT.
 * 
 *      This material has been approved for public release and unlimited
 *      distribution.
 **/

/**
 * @file ReferenceFrame.h
 * @author James Edmondson <jedmondson@gmail.com>
 *
 * This file contains the base reference Frame class
 **/

#ifndef _GAMS_POSE_REFERENCE_FRAME_H_
#define _GAMS_POSE_REFERENCE_FRAME_H_

#include <gams/GAMSExport.h>
#include <gams/CPP11_compat.h>
#include <vector>
#include <string>
#include <utility>
#include <stdexcept>
#include "ReferenceFrameFwd.h"
#include "CartesianFrame.h"
#include "Pose.h"

namespace gams
{
  namespace pose
  {
    /**
     * For internal use.
     *
     * Represents a frame's identity, persisting across timestamped versions,
     * including id and type.
     **/
    class GAMSExport ReferenceFrameIdentity
    {
    private:
        const ReferenceFrameType *type_;
        std::string id_;

    public:
        ReferenceFrameIdentity(const ReferenceFrameType *type,
                               std::string id)
          : type_(type), id_(std::move(id)) {}

        ReferenceFrameIdentity(const ReferenceFrameType *type)
          : type_(type), id_("DEFAULT") {}

        const ReferenceFrameType *type() const { return type_; }
        const std::string &id() const { return id_; }
    };

    /**
     * For internal use.
     *
     * Represents a specific frame version.
     **/
    class GAMSExport ReferenceFrameVersion
    {
    private:
      std::shared_ptr<ReferenceFrameIdentity> ident_;
      Pose origin_;
      uint64_t timestamp_ = -1;

    public:
      /**
       * Constructor from an origin, and optional timestamp. Will be
       * constructed with Cartesian type, and a random id.
       *
       * @tparam a Coordinate type convertible to Pose
       * @param origin the origin of this frame, relative to another frame.
       * @param timestamp the timestamp of this frame. By default, will be
       *        treated as "always most current".
       **/
      explicit ReferenceFrameVersion(
          Pose origin,
          uint64_t timestamp = -1)
        : ident_(std::make_shared<ReferenceFrameIdentity>(Cartesian)),
          origin_(std::move(origin)),
          timestamp_(timestamp) {}

      /**
       * Constructor from a type, an origin, and optional timestamp. Will be
       * constructed with a random id.
       *
       * @tparam a Coordinate type convertible to Pose
       * @param type a pointer to a ReferenceFrameType struct; typically,
       *        either Cartesian, or GPS.
       * @param origin the origin of this frame, relative to another frame.
       * @param timestamp the timestamp of this frame. By default, will be
       *        treated as "always most current".
       **/
      ReferenceFrameVersion(
          const ReferenceFrameType *type,
          Pose origin,
          uint64_t timestamp = -1)
        : ident_(std::make_shared<ReferenceFrameIdentity>(type)),
          origin_(std::move(origin)),
          timestamp_(timestamp) {}

      /**
       * Constructor from a id, an origin, and optional timestamp. Will be
       * constructed with Cartesian type.
       *
       * @tparam a Coordinate type convertible to Pose
       * @param id a string identifier for this frame.
       * @param origin the origin of this frame, relative to another frame.
       * @param timestamp the timestamp of this frame. By default, will be
       *        treated as "always most current".
       **/
      ReferenceFrameVersion(
          std::string name,
          Pose origin,
          uint64_t timestamp = -1)
        : ident_(std::make_shared<ReferenceFrameIdentity>(
              Cartesian,
              std::move(name))),
          origin_(std::move(origin)),
          timestamp_(timestamp) {}

      /**
       * Constructor from a type, id, an origin, and optional timestamp.
       *
       * @tparam a Coordinate type convertible to Pose
       * @param type a pointer to a ReferenceFrameType struct; typically,
       *        either Cartesian, or GPS.
       * @param id a string identifier for this frame.
       * @param origin the origin of this frame, relative to another frame.
       * @param timestamp the timestamp of this frame. By default, will be
       *        treated as "always most current".
       **/
      ReferenceFrameVersion(
          const ReferenceFrameType *type,
          std::string name,
          Pose origin,
          uint64_t timestamp = -1)
        : ident_(std::make_shared<ReferenceFrameIdentity>(
              type,
              std::move(name))),
          origin_(std::move(origin)),
          timestamp_(timestamp) {}

      /**
       * Constructor from an existing ReferenceFrameIdentity, an origin,
       * and optional timestamp. Typical users should not use this constructor.
       *
       * @tparam a Coordinate type convertible to Pose
       * @param ident shared_ptr to a ReferenceFrameIdentity, which holds
       *        type and id information.
       * @param id a string identifier for this frame.
       * @param origin the origin of this frame, relative to another frame.
       * @param timestamp the timestamp of this frame. By default, will be
       *        treated as "always most current".
       **/
      ReferenceFrameVersion(
          std::shared_ptr<ReferenceFrameIdentity> ident,
          Pose origin,
          uint64_t timestamp = -1)
        : ident_(std::move(ident)),
          origin_(std::move(origin)),
          timestamp_(timestamp) {}

      /**
       * Retrieve a reference to the ReferenceFrameIdentity object
       * holding the information common to all versions: id and type.
       *
       * @return a reference to the ReferenceFrameIdentity object
       **/
      const ReferenceFrameIdentity &ident() const { return *ident_; }

      /**
       * Retrieve the frame type object for this frame. Mostly useful for
       * comparing to the pose::Cartesian or pose::GPS instances to test
       * what kind of frame this is.
       *
       * @return a pointer to this frames ReferenceFrameType
       **/
      const ReferenceFrameType *type() const { return ident().type(); }

      /**
       * Gets the origin of this Frame
       *
       * @return the Pose which is the origin within this frame's parent,
       * or, a Pose within this own frame, with all zeros for coordinates,
       * if this frame has no parent.
       **/
      const Pose &origin() const {
        return origin_;
      }

      /**
       * Creates a new ReferenceFrame with modified origin
       *
       * @param new_origin the new origin
       * @return the new ReferenceFrame with new origin
       **/
      ReferenceFrame pose(Pose new_origin) const {
        return pose(new_origin, timestamp_);
      }

      /**
       * Creates a new ReferenceFrame with modified origin
       *
       * @param new_origin the new origin
       * @return the new ReferenceFrame with new origin
       **/
      ReferenceFrame move(Position new_origin) const {
        return move(new_origin, timestamp_);
      }

      /**
       * Creates a new ReferenceFrame with modified origin
       *
       * @param new_origin the new origin
       * @return the new ReferenceFrame with new origin
       **/
      ReferenceFrame orient(Orientation new_origin) const {
        return orient(new_origin, timestamp_);
      }

      /**
       * Creates a new ReferenceFrame with modified origin and timestamp
       *
       * @param new_origin the new origin
       * @param timestamp the new timestamp
       * @return the new ReferenceFrame with new origin and timestamp
       **/
      ReferenceFrame pose(const Pose &new_origin, uint64_t timestamp) const {
        return ReferenceFrame(ident_, new_origin, timestamp);
      }

      /**
       * Creates a new ReferenceFrame with modified origin and timestamp
       *
       * @param new_origin the new origin
       * @param timestamp the new timestamp
       * @return the new ReferenceFrame with new origin and timestamp
       **/
      ReferenceFrame move(const Position &new_origin,
                          uint64_t timestamp) const {
        return pose(Pose(new_origin, Orientation(origin_)), timestamp);
      }

      /**
       * Creates a new ReferenceFrame with modified origin and timestamp
       *
       * @param new_origin the new origin
       * @param timestamp the new timestamp
       * @return the new ReferenceFrame with new origin and timestamp
       **/
      ReferenceFrame orient(const Orientation &new_origin,
                            uint64_t timestamp) const {
        return pose(Pose(Position(origin_), new_origin), timestamp);
      }

      /**
       * Gets the parent frame (the one the origin is within). Will be *this
       * if no parent frame.
       **/
      ReferenceFrame origin_frame() const {
        return origin_.frame();
      }

      /**
       * Equality operator.
       *
       * @param other the frame to compare to.
       * @return true if both frames are the same object (i.e., same address).
       *    Otherwise, frames are not considered equal (returns false).
       **/
      bool operator==(const ReferenceFrame &other) const;

      /**
       * Inequality operator.
       *
       * @param other the frame to compare to.
       * @return false if both frames are the same object (i.e., same address).
       *    Otherwise, frames are not considered equal (returns true).
       **/
      bool operator!=(const ReferenceFrame &other) const;

      /**
       * Equality operator.
       *
       * @param other the frame to compare to.
       * @return true if both frames are the same object (i.e., same address).
       *    Otherwise, frames are not considered equal (returns false).
       **/
      bool operator==(const ReferenceFrameVersion &other) const;

      /**
       * Inequality operator.
       *
       * @param other the frame to compare to.
       * @return false if both frames are the same object (i.e., same address).
       *    Otherwise, frames are not considered equal (returns true).
       **/
      bool operator!=(const ReferenceFrameVersion &other) const;

      /**
       * Returns a human-readable name for the reference frame type
       *
       * @return the name reference frame type (e.g., GPS, Cartesian)
       **/
      const char *name() const {
        return ident().type()->name;
      }

      /**
       * Get the ID string of this frame. By default, frames generate a
       * random GUID as their ID
       **/
      const std::string &id() const {
        return ident_->id();
      }

      /**
       * Get the timestamp assigned to this frame.
       **/
      uint64_t timestamp() const {
        return timestamp_;
      }

      /**
       * Clone the this frame, but with new timestamp.
       *
       * @return the new frame object
       **/
      ReferenceFrame timestamp(uint64_t timestamp) const {
        return ReferenceFrame(ident_, origin_, timestamp);
      }

      /**
       * Returns true if this frame was interpolated from two stored frames.
       **/
      bool interpolated() const {
        return false;
      }

      /**
       * Save this ReferenceFrame to the knowledge base,
       * The saved frames will be marked with their timestamp for later
       * retrieval. If timestamp is -1, it will always be treated as the most
       * recent frame.
       *
       * @param kb the KnowledgeBase to store into
       **/
      void save(madara::knowledge::KnowledgeBase &kb) const {}

      /**
       * Load a single ReferenceFrame, by ID.
       *
       * @param id the ID of the frame to load
       * @param timestamp if -1, gets the latest frame (no interpolation)
       *   Otherwise, gets the frame at a specified timestamp,
       *   interpolated necessary.
       *
       * @return the imported ReferenceFrame, or an invalid frame if none
       *         exists.
       **/
      static ReferenceFrame load(
              madara::knowledge::KnowledgeBase &kb,
              const std::string &id,
              uint64_t timestamp = -1) { return default_frame(); }

      /**
       * Load ReferenceFrames, by ID, and their common ancestors. Will
       * interpolate frames to ensure the returned frames all have a common
       * timestamp.
       *
       * @tparam an InputIterator, of item type std::string
       *
       * @param begin beginning iterator
       * @param end ending iterator
       * @param timestamp if -1, the latest possible tree will be returned.
       *   Otherwise, the specified timestamp will be returned.
       *
       * @return a vector of ReferenceFrames, each corresponding to the
       *   input IDs, in the same order. If the timestamp specified cannot
       *   be satisfied, returns an empty vector.
       **/
      template<typename InputIterator>
      static std::vector<ReferenceFrame> load_tree(
              madara::knowledge::KnowledgeBase &kb,
              InputIterator begin,
              InputIterator end,
              uint64_t timestamp = -1) {
        size_t count = std::distance(begin, end);
        std::vector<ReferenceFrame> ret(count, default_frame());
        return ret;
      }

      /**
       * Load ReferenceFrames, by ID, and their common ancestors. Will
       * interpolate frames to ensure the returned frames all have a common
       * timestamp.
       *
       * @tparam a Container, supporting cbegin() and cend(),
       *    of item type std::string
       *
       * @param ids a Container of ids
       * @param timestamp if -1, the latest possible tree will be returned.
       *   Otherwise, the specified timestamp will be returned.
       *
       * @return a vector of ReferenceFrames, each corresponding to the
       *   input IDs, in the same order. If the timestamp specified cannot
       *   be satisfied, returns an empty vector.
       **/
      template<typename Container>
      static std::vector<ReferenceFrame> load_tree(
              madara::knowledge::KnowledgeBase &kb,
              const Container &ids,
              uint64_t timestamp = -1) {
        return load_tree(kb, ids.cbegin(), ids.cend(), timestamp);
      }

      /**
       * Save this ReferenceFrame to the knowledge base,
       * with a specific key value.
       *
       * @param kb the KnowledgeBase to save to
       * @param key a key prefix to save with
       **/
      void save_as(madara::knowledge::KnowledgeBase &kb,
                   const std::string &key) const {}

      /**
       * Load a ReferenceFrame using a specific key value (generally, one
       * previously used by export_tree_as). No interpolation will be done.
       *
       * @param kb the KnowledgeBase to load from
       * @param key a key prefix to load with
       * @returns a pointer to the imported ReferenceFrame, or nullptr if
       * none exists.
       **/
      static ReferenceFrame load_as(madara::knowledge::KnowledgeBase &kb,
                                    const std::string &key) {
        return default_frame();
      }

      template<typename CoordType>
      friend class Coordinate;
    };

    /**
     * Helper function to find the common frame between two frames.
     *
     * @param from the initial frame
     * @param to the target frame
     * @param to_stack if not nullptr, the frames needed to go from base to
     *  target frame will be pushed to pointed to vector
     **/
    const ReferenceFrame *find_common_frame(
        const ReferenceFrame *from,
        const ReferenceFrame *to,
        std::vector<const ReferenceFrame *> *to_stack = nullptr);

    /**
     * Thrown when an an attempt is made to transform between frames
     * that do not belong to the same frame tree.
     **/
    class unrelated_frames : public std::runtime_error
    {
    public:
      /**
       * The only Constructor.
       *
       * @param from_frame the frame the coordinate belongs to
       * @param to_frame the frame the coordinate is being transformed to
       **/
      unrelated_frames(ReferenceFrame from_frame, ReferenceFrame to_frame);
      ~unrelated_frames() throw();

      ReferenceFrame from_frame;
      ReferenceFrame to_frame;
    };

    /**
     * Thrown when an attempt is made to transform between two frame types,
     * and there's not transform defined.
     *
     * Note that between two frame types A and B, there are four different
     * kinds of transforms, which must be defined individually:
     *    1) From A as parent to B as its child
     *    2) From A as child to B as its parent
     *    3) From B as parent to A as its child
     *    4) From B as child to A as its parent
     **/
    class undefined_transform : public std::runtime_error
    {
    public:
      /**
       * The only Constructor
       *
       * @param parent_frame of the two involved frames, the parent frame
       * @param child_frame of the two involved frames, the child frame
       * @param is_child_to_parent indicates direction of transformation
       * @param unsupported_angular true if the error was due to orientd
       *   reference frames not being supported for this transformation.
       *   Defaults to false;
       **/
      undefined_transform(
        const ReferenceFrameType *parent_frame,
        const ReferenceFrameType *child_frame,
        bool is_child_to_parent,
        bool unsupported_angular = false);
      ~undefined_transform() throw();

      const ReferenceFrameType *parent_frame;
      const ReferenceFrameType *child_frame;
      bool is_child_to_parent;
      bool unsupported_angular;
    };

    /**
     * For internal use.
     *
     * Provides implementation of angular and pose transforms for frames
     * where angular transformation is independant of linear. This applies
     * to, for example, Cartesian and GPS frames, but not UTM frames.
     * Inherit from this to use this implementation.
     **/
    namespace simple_rotate {
      /**
       * Rotates a LinearVector according to a AngularVector
       *
       * @param x   the x coordinate to orient (in-place)
       * @param y   the y coordinate to orient (in-place)
       * @param z   the z coordinate to orient (in-place)
       * @param rot the angular to apply, axis-angle notation
       * @param reverse if true, apply angular in opposite direction
       **/
      void orient_linear_vec(
          double &x, double &y, double &z,
          double rx, double ry, double rz,
          bool reverse = false);

      /**
       * Transform AngularVector in-place into its origin frame from this frame
       *
       * @param rx  the x component of the axis-angle representation
       * @param ry  the y component of the axis-angle representation
       * @param rz  the z component of the axis-angle representation
       **/
      void transform_angular_to_origin(
                      const ReferenceFrameType *origin,
                      const ReferenceFrameType *self,
                      double orx, double ory, double orz,
                      double &rx, double &ry, double &rz);

      /**
       * Transform AngularVector in-place from its origin frame
       *
       * @param rx  the x component of the axis-angle representation
       * @param ry  the y component of the axis-angle representation
       * @param rz  the z component of the axis-angle representation
       **/
      void transform_angular_from_origin(
                      const ReferenceFrameType *origin,
                      const ReferenceFrameType *self,
                      double orx, double ory, double orz,
                      double &rx, double &ry, double &rz);

      /**
       * Transform pose in-place into its origin frame from this frame.
       * Simply applies linear and angular transforms independantly
       *
       * @param x the x axis for the coordinate to translate
       * @param y the y axis for the coordinate to translate
       * @param z the z axis for the coordinate to translate
       * @param rx  the x component of the axis-angle representation
       * @param ry  the y component of the axis-angle representation
       * @param rz  the z component of the axis-angle representation
       **/
      void transform_pose_to_origin(
                      const ReferenceFrameType *origin,
                      const ReferenceFrameType *self,
                      double ox, double oy, double oz,
                      double orx, double ory, double orz,
                      double &x, double &y, double &z,
                      double &rx, double &ry, double &rz);

      /**
       * Transform pose in-place from its origin frame
       * Simply applies linear and angular transforms independantly
       *
       * @param x the x axis for the coordinate to translate
       * @param y the y axis for the coordinate to translate
       * @param z the z axis for the coordinate to translate
       * @param rx  the x component of the axis-angle representation
       * @param ry  the y component of the axis-angle representation
       * @param rz  the z component of the axis-angle representation
       **/
      void transform_pose_from_origin(
                      const ReferenceFrameType *origin,
                      const ReferenceFrameType *self,
                      double ox, double oy, double oz,
                      double orx, double ory, double orz,
                      double &x, double &y, double &z,
                      double &rx, double &ry, double &rz);

      /**
       * Calculates smallest angle between two AngularVectors
       *
       * @param rx1  the starting angular on x axis
       * @param ry1  the starting angular on y axis
       * @param rz1  the starting angular on z axis
       * @param rx2 the ending rotatation on x axis
       * @param ry2  the ending angular on y axis
       * @param rz2  the ending angular on z axis
       * @return the difference in radians
       **/
      double calc_angle(
                      const ReferenceFrameType *self,
                      double rx1, double ry1, double rz1,
                      double rx2, double ry2, double rz2);
    }

    inline void default_normalize_linear(
                const ReferenceFrameType *,
                double &, double &, double &) {}

    inline void default_normalize_angular(
                const ReferenceFrameType *,
                double &, double &, double &) {}

    inline void default_normalize_pose(
                    const ReferenceFrameType *self,
                    double &x, double &y, double &z,
                    double &rx, double &ry, double &rz)
    {
      self->normalize_linear(self, x, y, z);
      self->normalize_angular(self, rx, ry, rz);
    }
  }
}

#include "ReferenceFrame.inl"

#endif
