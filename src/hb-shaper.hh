/*
 * Copyright © 2012  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_SHAPER_HH
#define HB_SHAPER_HH

#include "hb.hh"
#include "hb-machinery.hh"

typedef hb_bool_t hb_shape_func_t (hb_shape_plan_t    *shape_plan,
				   hb_font_t          *font,
				   hb_buffer_t        *buffer,
				   const hb_feature_t *features,
				   unsigned int        num_features);

#define HB_SHAPER_IMPLEMENT(name) \
	extern "C" HB_INTERNAL hb_shape_func_t _hb_##name##_shape;
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT

struct hb_shaper_entry_t {
  char name[16];
  hb_shape_func_t *func;
};

HB_INTERNAL const hb_shaper_entry_t *
_hb_shapers_get (void);


#if 0
#define HB_SHAPER_DATA_ENSURE_DEFINE_WITH_CONDITION(shaper, object, condition) \
bool \
HB_SHAPER_DATA_ENSURE_FUNC(shaper, object) (hb_##object##_t *object) \
{\
  retry: \
  HB_SHAPER_DATA_TYPE (shaper, object) *data = HB_SHAPER_DATA (shaper, object).get (); \
  if (likely (data) && !(condition)) { \
    /* XXX-MT-bug \
     * Note that evaluating condition above can be dangerous if another thread \
     * got here first and destructed data.  That's, as always, bad use pattern. \
     * If you modify the font (change font size), other threads must not be \
     * using it at the same time.  However, since this check is delayed to \
     * when one actually tries to shape something, this is a XXX race condition \
     * (and the only know we have that I know of) right now.  Ie. you modify the \
     * font size in one thread, then (supposedly safely) try to use it from two \
     * or more threads and BOOM!  I'm not sure how to fix this.  We want RCU. \
     * Maybe when it doesn't matter when we finally implement AAT shaping, as
     * this (condition) is currently only used by hb-coretext. */ \
    /* Drop and recreate. */ \
    /* If someone dropped it in the mean time, throw it away and don't touch it. \
     * Otherwise, destruct it. */ \
    if (likely (HB_SHAPER_DATA (shaper, object).cmpexch (data, nullptr))) \
    { \
      HB_SHAPER_DATA_DESTROY_FUNC (shaper, object) (data); \
    } \
    goto retry; \
  } \
  if (unlikely (!data)) { \
    data = HB_SHAPER_DATA_CREATE_FUNC (shaper, object) (object); \
    if (unlikely (!data)) \
      data = (HB_SHAPER_DATA_TYPE (shaper, object) *) HB_SHAPER_DATA_INVALID; \
    if (unlikely (!HB_SHAPER_DATA (shaper, object).cmpexch (nullptr, data))) { \
      if (data) \
	HB_SHAPER_DATA_DESTROY_FUNC (shaper, object) (data); \
      goto retry; \
    } \
  } \
  return data != nullptr && (void *) data != HB_SHAPER_DATA_INVALID; \
} \
static_assert (true, "") /* Require semicolon. */
#endif


template <typename Data, unsigned int WheresData, typename T>
struct hb_shaper_lazy_loader_t;

#define HB_SHAPER_ORDER(Shaper) \
  HB_PASTE (HB_SHAPER_ORDER_, Shaper)
enum hb_shaper_order_t
{
  _HB_SHAPER_ORDER_ORDER_ZERO,
#define HB_SHAPER_IMPLEMENT(Shaper) \
      HB_SHAPER_ORDER (Shaper),
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
  _HB_SHAPERS_COUNT_PLUS_ONE,
  HB_SHAPERS_COUNT = _HB_SHAPERS_COUNT_PLUS_ONE - 1,
};

template <enum hb_shaper_order_t order, typename Object> struct hb_shaper_object_data_type_t;

#define HB_SHAPER_DATA_SUCCEEDED ((void *) +1)
#define HB_SHAPER_DATA_TYPE(shaper, object)		hb_##shaper##_##object##_data_t
#define HB_SHAPER_DATA_CREATE_FUNC(shaper, object)	_hb_##shaper##_shaper_##object##_data_create
#define HB_SHAPER_DATA_DESTROY_FUNC(shaper, object)	_hb_##shaper##_shaper_##object##_data_destroy

#define HB_SHAPER_DATA_INSTANTIATE_SHAPERS(shaper, object) \
	\
	struct HB_SHAPER_DATA_TYPE (shaper, object); /* Type forward declaration. */ \
	extern "C" HB_INTERNAL HB_SHAPER_DATA_TYPE (shaper, object) * \
	HB_SHAPER_DATA_CREATE_FUNC (shaper, object) (hb_##object##_t *object); \
	extern "C" HB_INTERNAL void \
	HB_SHAPER_DATA_DESTROY_FUNC (shaper, object) (HB_SHAPER_DATA_TYPE (shaper, object) *shaper##_##object); \
	\
	template <> \
	struct hb_shaper_object_data_type_t<HB_SHAPER_ORDER (shaper), hb_##object##_t> \
	{ \
	  typedef HB_SHAPER_DATA_TYPE(shaper, object) value; \
	}; \
	\
	template <unsigned int WheresData> \
	struct hb_shaper_lazy_loader_t<hb_##object##_t, WheresData, HB_SHAPER_DATA_TYPE(shaper, object)> \
		: hb_lazy_loader_t<HB_SHAPER_DATA_TYPE(shaper, object), \
				   hb_shaper_lazy_loader_t<hb_##object##_t, \
							   WheresData, \
							   HB_SHAPER_DATA_TYPE(shaper, object)>, \
				   hb_##object##_t, WheresData> \
	{ \
	  typedef HB_SHAPER_DATA_TYPE(shaper, object) Type; \
	  static inline Type* create (hb_##object##_t *data) \
	  { return HB_SHAPER_DATA_CREATE_FUNC (shaper, object) (data); } \
	  static inline Type *get_null (void) { return nullptr; } \
	  static inline void destroy (Type *p) { HB_SHAPER_DATA_DESTROY_FUNC (shaper, object) (p); } \
	}; \
	\
	static_assert (true, "") /* Require semicolon. */


template <typename Object>
struct hb_shaper_object_dataset_t
{
  inline void init0 (Object *parent_data)
  {
    this->parent_data = parent_data;
#define HB_SHAPER_IMPLEMENT(shaper) shaper.init0 ();
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
  }
  inline void fini (void)
  {
#define HB_SHAPER_IMPLEMENT(shaper) shaper.fini ();
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
  }

  Object *parent_data; /* MUST be JUST before the lazy loaders. */
#define HB_SHAPER_IMPLEMENT(shaper) \
	hb_shaper_lazy_loader_t<Object, HB_SHAPER_ORDER(shaper), \
				typename hb_shaper_object_data_type_t<HB_SHAPER_ORDER(shaper), Object>::value \
			       > shaper;
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
};

#endif /* HB_SHAPER_HH */
