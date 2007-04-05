#ifdef MCXX_REFCOUNT_DEBUG
  #include <stdio.h>
#endif
#include <string.h>
#include "mcxx_refcount.h"

/*
 * Implemented according to 
 *
 * "Concurrent Cycle Collection in Reference Counted Systems", David F. Bacon,
 * V.T. Rajan in Proceedings European Conference on OOP June 2001, LNCS, vol
 * 2072
 *
 * This file only implements de the synchronous collection
 *
 * Roger Ferrer Ib��ez - 2007
 *
 */

static void _mcxx_nochildren(void *p, void (*_mcxx_do)(void*));

static void _mcxx_release(void *p);
static void _mcxx_possible_root(void *p);
static void _mcxx_append_to_roots(void *p);
static void _mcxx_remove_from_roots(void *p);
static void _mcxx_markroots(void);
static void _mcxx_scanroots(void);
static void _mcxx_collectroots(void);
static void _mcxx_markgray(void *p);
static void _mcxx_collectwhite(void *p);
static void _mcxx_scan(void *p);
static void _mcxx_scanblack(void *p);

#define MCXX_MAX_ROOTS (256)
static int _mcxx_numroots = 0;
static void *_mcxx_roots[MCXX_MAX_ROOTS];

void *_mcxx_calloc(size_t nmemb, size_t size)
{
    void *p = calloc(nmemb, size);

#ifdef MCXX_REFCOUNT_DEBUG
    fprintf(stderr, "%s Creating new object %p\n", __FUNCTION__, p);
#endif

    _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);
    
    // Set by default to nonchildren
    refp->_mcxx_children = _mcxx_nochildren;

    return p;
}

static void _mcxx_nochildren(void *p, void (*_mcxx_do)(void*))
{
    // Do nothing
}

void _mcxx_increment(void *p)
{
    _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);

#ifdef MCXX_REFCOUNT_DEBUG
    fprintf(stderr, "%s Incrementing count of %p\n", __FUNCTION__, p);
#endif

    refp->_mcxx_refcount++;
    refp->_mcxx_colour = _MCXX_BLACK;
}

void _mcxx_decrement(void *p)
{
    _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);

    // Reduce the reference counter
    refp->_mcxx_refcount--;

#ifdef MCXX_REFCOUNT_DEBUG
    fprintf(stderr, "%s Decrementing count of %p\n", __FUNCTION__, p);
#endif

    // If no references where hold
    if (refp->_mcxx_refcount == 0)
    {
        // Release it
#ifdef MCXX_REFCOUNT_DEBUG
    fprintf(stderr, "%s Count of %p is zero, releasing it\n", __FUNCTION__, p);
#endif
        _mcxx_release(p);
    }
    else
    {
        // Otherwise mark it as a possible root
#ifdef MCXX_REFCOUNT_DEBUG
    fprintf(stderr, "%s Marking %p as a possible root\n", __FUNCTION__, p);
#endif
        _mcxx_possible_root(p);
    }
}

static void _mcxx_release(void *p)
{
    _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);

    // Recursively decrement all the childrens of refp
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Decrementing count of children of %p\n", __FUNCTION__, p);
#endif
    (refp->_mcxx_children)(refp, _mcxx_decrement);
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Children of %p decremented\n", __FUNCTION__, p);
#endif

#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Setting %p as black\n", __FUNCTION__, p);
#endif
    refp->_mcxx_colour = _MCXX_BLACK;

    // If not buffered free
    if (!refp->_mcxx_buffered)
    {
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Freeing non-buffered %p\n", __FUNCTION__, p);
#endif
        free(p);
    }
#ifdef MCXX_REFCOUNT_DEBUG
    else
    {
        fprintf(stderr, "%s Not freeing %p since it is buffered\n", __FUNCTION__, p);
    }
#endif
}

static void _mcxx_possible_root(void *p)
{
    _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);

    if (refp->_mcxx_colour != _MCXX_PURPLE)
    {
        refp->_mcxx_colour = _MCXX_PURPLE;
        if (!refp->_mcxx_buffered)
        {
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Setting %p as buffered and appending it to roots\n", __FUNCTION__, p);
#endif
            refp->_mcxx_buffered = 1;
            _mcxx_append_to_roots(p);
        }
    }
}


void _mcxx_collectcycles(void)
{
    _mcxx_markroots();
    _mcxx_scanroots();
    _mcxx_collectroots();
}


static void _mcxx_markroots(void)
{
    int numroots = _mcxx_numroots;
    void *roots[MCXX_MAX_ROOTS];
    memcpy(roots, _mcxx_roots, sizeof(_mcxx_roots));

    int i;
    for (i = 0; i < numroots; i++)
    {
        void *p = roots[i];
        _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Considering object %p (%d of %d)\n", __FUNCTION__, p, i, numroots);
#endif

        if (refp->_mcxx_colour == _MCXX_PURPLE)
        {
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Object %p is purple, marking it gray\n", __FUNCTION__, p);
#endif
            _mcxx_markgray(p);
        }
        else
        {
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Object %p is not purple, marking not buffered and removing from roots\n", __FUNCTION__, p);
#endif
            refp->_mcxx_buffered = 0;
            _mcxx_remove_from_roots(p);
            if (refp->_mcxx_colour == _MCXX_BLACK
                    && refp->_mcxx_refcount == 0)
            {
#ifdef MCXX_REFCOUNT_DEBUG
                fprintf(stderr, "%s Freeing %p because of it being black and refcount zero\n", __FUNCTION__, p);
#endif
                free(p);
            }

        }
    }
}

static void _mcxx_scanroots(void)
{
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Scanning roots\n", __FUNCTION__);
#endif
    int numroots = _mcxx_numroots;
    void *roots[MCXX_MAX_ROOTS];
    memcpy(roots, _mcxx_roots, sizeof(_mcxx_roots));

    int i;
    for (i = 0; i < numroots; i++)
    {
        void *p = roots[i];

        _mcxx_scan(p);
    }
}

static void _mcxx_collectroots(void)
{
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Collecting roots\n", __FUNCTION__);
#endif
    int numroots = _mcxx_numroots;
    void *roots[MCXX_MAX_ROOTS];
    memcpy(roots, _mcxx_roots, sizeof(_mcxx_roots));

    int i;
    for (i = 0; i < numroots; i++)
    {
        void *p = roots[i];
        _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);

#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Removing %p from roots and setting not buffered\n", __FUNCTION__, p);
#endif
        _mcxx_remove_from_roots(p);
        refp->_mcxx_buffered = 0;
        _mcxx_collectwhite(p);
    }
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Roots collected\n", __FUNCTION__);
#endif
}

// Auxiliar function for _mcxx_markgray
static void _mcxx_markgray_aux(void *p)
{
    _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Decreasing counter of %p\n", __FUNCTION__, p);
#endif
    refp->_mcxx_refcount--;
    _mcxx_markgray(p);
}

static void _mcxx_markgray(void *p)
{
    _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);

    if (refp->_mcxx_colour != _MCXX_GRAY)
    {
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Marking %p as gray\n", __FUNCTION__, p);
#endif
        refp->_mcxx_colour = _MCXX_GRAY;

        // For every children, reduce its refcount and markgray
        // recursively
        (refp->_mcxx_children)(p, _mcxx_markgray_aux);
    }
}

static void _mcxx_scan(void *p)
{
    _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);

    if (refp->_mcxx_colour == _MCXX_GRAY)
    {
        if (refp->_mcxx_refcount > 0)
        {
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Scanning blacks of %p since gray and refcount > 0\n", __FUNCTION__, p);
#endif
            _mcxx_scanblack(p);
        }
        else // refcount == 0
        {
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Setting %p as white and scanning children since it is gray and refcount == 0\n", __FUNCTION__, p);
#endif
            refp->_mcxx_colour = _MCXX_WHITE;

            // For every children scan it
            (refp->_mcxx_children)(p, _mcxx_scan);
        }
    }
}

// Auxiliar function for _mcxx_scanblack
static void _mcxx_scanblack_aux(void *p)
{
    _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);

#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Increasign refcount of %p\n", __FUNCTION__, p);
#endif
    refp->_mcxx_refcount++;
    if (refp->_mcxx_colour != _MCXX_BLACK)
    {
        _mcxx_scanblack(p);
    }
}

static void _mcxx_scanblack(void *p)
{
    _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);

#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Scanning black %p\n", __FUNCTION__, p);
#endif

#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Setting %p as black\n", __FUNCTION__, p);
#endif
    refp->_mcxx_colour = _MCXX_BLACK;

    // For every child reduce increase the reference counter and if not black
    // scan their blacks
    (refp->_mcxx_children)(p, _mcxx_scanblack_aux);
}

static void _mcxx_collectwhite(void *p)
{
    _p_mcxx_base_refcount_t refp = (_p_mcxx_base_refcount_t)(p);

    if (refp->_mcxx_colour == _MCXX_WHITE
            && !refp->_mcxx_buffered)
    {
        refp->_mcxx_colour = _MCXX_BLACK;
        (refp->_mcxx_children)(p, _mcxx_collectwhite);

#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Freeing since white and not buffered %p\n", __FUNCTION__, p);
#endif
        free(p);
    }
}

static void _mcxx_append_to_roots(void *p)
{
#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Adding %p to roots\n", __FUNCTION__, p);
#endif
    _mcxx_roots[_mcxx_numroots] = p;
    _mcxx_numroots++;

    // ??
    if (_mcxx_numroots == MCXX_MAX_ROOTS)
    {
        _mcxx_collectcycles();
    }
}

static void _mcxx_remove_from_roots(void *p)
{
    // First find in the list of roots
    int i;
    int index = -1;
    for (i = 0; i < _mcxx_numroots; i++)
    {
        if (_mcxx_roots[i] == p)
        {
            index = i;
            break;
        }
    }

    if (index < 0)
        return;

#ifdef MCXX_REFCOUNT_DEBUG
        fprintf(stderr, "%s Removing %p from roots\n", __FUNCTION__, p);
#endif

    // Shift them
    for (i = (index + 1); i < _mcxx_numroots; i++)
    {
        _mcxx_roots[i-1] = _mcxx_roots[i];
    }
    
    // And decrease
    _mcxx_numroots--;
}
