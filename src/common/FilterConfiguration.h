/*
** Surge Synthesizer is Free and Open Source Software
**
** Surge is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html
**
** Copyright 2004-2020 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Surge was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Surge
** open source in September 2018.
*/

#include "Parameter.h"
#pragma once

#include "sst/filters.h"

/*
 * OK so what the heck is happening here, you may ask? Well, let me explain. Grab a cup of tea.
 *
 * As we approached Surge 1.8 with new filters, we realized our filter list was getting
 * long. So we decided to use submenus. But when doing that, we realized that the
 * natural grouping of filters (LP, BP, HP, Notch, Special...) didn't work with the way
 * our filter models had worked. We had things like "bandpass doesn't split 12 dB/24 dB" into
 * separate types, and "OB-Xd is kinda all sorts of filters". So, over in issue #3006,
 * we decided that some filters needed splitting, kinda like what Luna had done in the non-linear
 * feedback set. But splitting means that subtype counts change and streaming breaks.
 * And we wanted to split filters which weren't in streaming version 14 (1.7->1.8) either.
 *
 * So what we did was add in streaming version 15 a "post patch streaming fixup" operation
 * which allows you to see the prior version, the current version, and adjust. That way we can
 * do things like "OB-Xd subtype 7 in streaming version 14 is actually OB-Xd highpass subtype 3
 * in streaming version 15 and above". Or whatever.
 *
 * But to do *that*, we need to keep the old enums around so we can write that code. So these
 * are the old enums.
 *
 * Then the only question left is - how to split? I chose the 'add at end for splits' method. That
 * is, fut_14_bp12 splits into fut_bp12 and fut_bp24, but I added fut_bp24 at the end of the list.
 * Pros and cons: if I added it adjacent, the names in the name array would line up, but the
 * remapping code would be wildly more complicated. I chose simple remapping code (that is S&H and
 * vintage ladder are no-ops in remap) at the cost of an oddly ordered filter name list. That's the
 * right choice, but when you curse me for the odd name list, you can come back and read this
 * comment and feel slightly better. Finally, items which split and changed meaning got a new name
 * (so fut_comb is now fut_comb_pos and fut_comb_neg, say), which requires us to go and fix up any
 * code which referred to the old values.
 */

enum fu_type_sv14
{
    fut_14_none = 0,
    fut_14_lp12,
    fut_14_lp24,
    fut_14_lpmoog,
    fut_14_hp12,
    fut_14_hp24,
    fut_14_bp12,
    fut_14_notch12,
    fut_14_comb,
    fut_14_SNH,
    fut_14_vintageladder,
    fut_14_obxd_2pole,
    fut_14_obxd_4pole,
    fut_14_k35_lp,
    fut_14_k35_hp,
    fut_14_diode,
    fut_14_cutoffwarp_lp,
    fut_14_cutoffwarp_hp,
    fut_14_cutoffwarp_n,
    fut_14_cutoffwarp_bp,
    n_fu_14_types,
};

struct FilterSelectorMapper : public ParameterDiscreteIndexRemapper
{
    std::vector<std::pair<int, std::string>> mapping;
    std::unordered_map<int, int> inverseMapping;
    void p(int i, const std::string &s) { mapping.push_back(std::make_pair(i, s)); }
    FilterSelectorMapper()
    {
        using sst::filters::FilterType;

        p(FilterType::fut_none, "");

        p(FilterType::fut_lp12, "Lowpass");
        p(FilterType::fut_lp24, "Lowpass");
        p(FilterType::fut_lpmoog, "Lowpass");
        p(FilterType::fut_vintageladder, "Lowpass");
        p(FilterType::fut_k35_lp, "Lowpass");
        p(FilterType::fut_diode, "Lowpass");
        p(FilterType::fut_obxd_2pole_lp, "Lowpass"); // ADJ
        p(FilterType::fut_obxd_4pole, "Lowpass");
        p(FilterType::fut_cutoffwarp_lp, "Lowpass");
        p(FilterType::fut_resonancewarp_lp, "Lowpass");

        p(FilterType::fut_bp12, "Bandpass");
        p(FilterType::fut_bp24, "Bandpass");
        p(FilterType::fut_obxd_2pole_bp, "Bandpass");
        p(FilterType::fut_cutoffwarp_bp, "Bandpass");
        p(FilterType::fut_resonancewarp_bp, "Bandpass");

        p(FilterType::fut_hp12, "Highpass");
        p(FilterType::fut_hp24, "Highpass");
        p(FilterType::fut_k35_hp, "Highpass");
        p(FilterType::fut_obxd_2pole_hp, "Highpass");
        p(FilterType::fut_cutoffwarp_hp, "Highpass");
        p(FilterType::fut_resonancewarp_hp, "Highpass");

        p(FilterType::fut_notch12, "Notch");
        p(FilterType::fut_notch24, "Notch");
        p(FilterType::fut_obxd_2pole_n, "Notch");
        p(FilterType::fut_cutoffwarp_n, "Notch");
        p(FilterType::fut_resonancewarp_n, "Notch");

        p(FilterType::fut_tripole, "Multi");

        p(FilterType::fut_apf, "Effect");
        p(FilterType::fut_cutoffwarp_ap, "Effect");
        p(FilterType::fut_resonancewarp_ap, "Effect");
        p(FilterType::fut_comb_pos, "Effect");
        p(FilterType::fut_comb_neg, "Effect");
        p(FilterType::fut_SNH, "Effect");

        int c = 0;
        for (auto e : mapping)
            inverseMapping[e.first] = c++;

        if (mapping.size() != FilterType::num_filter_types)
            std::cout << "BAD MAPPING TYPES" << std::endl;
    }

    virtual int remapStreamedIndexToDisplayIndex(int i) const override
    {
        return inverseMapping.at(i);
    }
    virtual std::string nameAtStreamedIndex(int i) const override
    {
        return sst::filters::filter_menu_names[i];
    }
    virtual bool hasGroupNames() const override { return true; };

    virtual std::string groupNameAtStreamedIndex(int i) const override
    {
        return mapping[inverseMapping.at(i)].second;
    }

    virtual bool sortGroupNames() const override { return false; }
    bool useRemappedOrderingForGroupsIfNotSorted() const override { return true; }

    virtual bool supportsTotalIndexOrdering() const override { return true; }
    virtual const std::vector<int> totalIndexOrdering() const override
    {
        auto res = std::vector<int>();
        for (auto m : mapping)
            res.push_back(m.first);
        return res;
    }
};

/*
 * Finally we need to map streaming indices to positions on the glyph display. This
 * should *really* be in UI code but it is just a declaration and having all the declarations
 * together is useful. In the far distant future perhaps we customize this by skin.
 */

const int lprow = 1;
const int bprow = 2;
const int hprow = 3;
const int nrow = 4;
const int multirow = 5;
const int fxrow = 6;

const int fut_glyph_index[sst::filters::num_filter_types][2] = {
    {0, 0},        // fut_none
    {0, lprow},    // fut_lp12
    {1, lprow},    // fut_lp24
    {3, lprow},    // fut_lpmoog
    {0, hprow},    // fut_hp12
    {1, hprow},    // fut_hp24
    {0, bprow},    // fut_bp12
    {0, nrow},     // fut_notch12
    {1, fxrow},    // fut_comb_pos
    {3, fxrow},    // fut_SNH
    {4, lprow},    // fut_vintageladder
    {6, lprow},    // fut_obxd_2pole
    {7, lprow},    // fut_obxd_4pole
    {2, lprow},    // fut_k35_lp
    {2, hprow},    // fut_k35_hp
    {5, lprow},    // fut_diode
    {8, lprow},    // fut_cutoffwarp_lp
    {4, hprow},    // fut_cutoffwarp_hp
    {3, nrow},     // fut_cutoffwarp_n
    {3, bprow},    // fut_cutoffwarp_bp
    {3, hprow},    // fut_obxd_2pole_hp,
    {2, nrow},     // fut_obxd_2pole_n,
    {2, bprow},    // fut_obxd_2pole_bp,
    {1, bprow},    // fut_bp24,
    {1, nrow},     // fut_notch24,
    {2, fxrow},    // fut_comb_neg,
    {0, fxrow},    // fut_apf
    {0, fxrow},    // fut_cutoffwarp_ap (this is temporarily set to just use the regular AP glyph)
    {9, lprow},    // fut_resonancewarp_lp
    {5, hprow},    // fut_resonancewarp_hp
    {4, nrow},     // fut_resonancewarp_n
    {4, bprow},    // fut_resonancewarp_bp
    {0, fxrow},    // fut_resonancewarp_ap (also temporarily set to just use the regular AP glyph)
    {0, multirow}, // fut_tripole
};

enum ws_type
{
    wst_none = 0,
    wst_soft,
    wst_hard,
    wst_asym,
    wst_sine,
    wst_digital,

    // XT waves start here
    wst_cheby2,
    wst_cheby3,
    wst_cheby4,
    wst_cheby5,

    wst_fwrectify,
    wst_poswav,
    wst_negwav,
    wst_softrect,

    wst_singlefold,
    wst_dualfold,
    wst_westfold,

    // additive harmonics
    wst_add12,
    wst_add13,
    wst_add14,
    wst_add15,
    wst_add12345,
    wst_addsaw3,
    wst_addsqr3,

    wst_fuzz,
    wst_fuzzsoft,
    wst_fuzzheavy,
    wst_fuzzctr,
    wst_fuzzsoftedge,

    wst_sinpx,
    wst_sin2xpb,
    wst_sin3xpb,
    wst_sin7xpb,
    wst_sin10xpb,

    wst_2cyc,
    wst_7cyc,
    wst_10cyc,

    wst_2cycbound,
    wst_7cycbound,
    wst_10cycbound,

    wst_zamsat,
    wst_ojd,

    wst_softfold,

    n_ws_types,
};

const char wst_names[n_ws_types][32] = {"Off",
                                        "Soft",
                                        "Hard",
                                        "Asymmetric",
                                        "Sine",
                                        "Digital",
                                        "Soft Harmonic 2",
                                        "Soft Harmonic 3",
                                        "Soft Harmonic 4",
                                        "Soft Harmonic 5",
                                        "Full Wave",
                                        "Half Wave Positive",
                                        "Half Wave Negative",
                                        "Soft Rectifier",
                                        "Single Fold",
                                        "Double Fold",
                                        "West Coast Fold",
                                        "Additive 1+2",
                                        "Additive 1+3",
                                        "Additive 1+4",
                                        "Additive 1+5",
                                        "Additive 12345",
                                        "Additive Saw 3",
                                        "Additive Square 3",

                                        "Fuzz",
                                        "Fuzz Soft Clip",
                                        "Heavy Fuzz",
                                        "Fuzz Center",
                                        "Fuzz Soft Edge",

                                        "Sin+x",
                                        "Sin 2x + x",
                                        "Sin 3x + x",
                                        "Sin 7x + x",
                                        "Sin 10x + x",
                                        "2 Cycle",
                                        "7 Cycle",
                                        "10 Cycle",
                                        "2 Cycle Bound",
                                        "7 Cycle Bound",
                                        "10 Cycle Bound",
                                        "Medium",
                                        "OJD",
                                        "Soft Single Fold"};

const char wst_ui_names[n_ws_types][16] = {
    "Off",     "Soft",     "Hard",     "Asym",     "Sine",    "Digital",  "Harm 2",  "Harm 3",
    "Harm 4",  "Harm 5",   "FullRect", "HalfPos",  "HalfNeg", "SoftRect", "1Fold",   "2Fold",
    "WCFold",  "Add12",    "Add13",    "Add14",    "Add15",   "Add1-5",   "AddSaw3", "AddSqr3",
    "Fuzz",    "SoftFz",   "HeavyFz",  "CenterFz", "EdgeFz",  "Sin+x",    "Sin2x+x", "Sin3x+x",
    "Sin7x+x", "Sin10x+x", "2Cycle",   "7Cycle",   "10Cycle", "2CycleB",  "7CycleB", "10CycleB",
    "Medium",  "OJD",      "Sft1Fld"};

// Subset used in distortion and rotary
static constexpr int n_fxws = 8;
static constexpr std::array<ws_type, n_fxws> FXWaveShapers = {
    wst_soft,    wst_hard,      wst_asym,    wst_sine,
    wst_digital, // If you adjust this list above here, you break 1.9 patch compat
    wst_ojd,     wst_fwrectify, wst_fuzzsoft};

struct WaveShaperSelectorMapper : public ParameterDiscreteIndexRemapper
{
    std::vector<std::pair<int, std::string>> mapping;
    std::unordered_map<int, int> inverseMapping;
    void p(int i, const std::string &s) { mapping.emplace_back(i, s); }
    WaveShaperSelectorMapper()
    {
        // Obviously improve this
        p(wst_none, "");

        p(wst_soft, "Saturator");
        p(wst_zamsat, "Saturator");
        p(wst_hard, "Saturator");
        p(wst_asym, "Saturator");
        p(wst_ojd, "Saturator");

        p(wst_sine, "Effect");
        p(wst_digital, "Effect");

        p(wst_cheby2, "Harmonic");
        p(wst_cheby3, "Harmonic");
        p(wst_cheby4, "Harmonic");
        p(wst_cheby5, "Harmonic");

        p(wst_add12, "Harmonic");
        p(wst_add13, "Harmonic");
        p(wst_add14, "Harmonic");
        p(wst_add15, "Harmonic");
        p(wst_add12345, "Harmonic");
        p(wst_addsaw3, "Harmonic");
        p(wst_addsqr3, "Harmonic");

        p(wst_fwrectify, "Rectifiers");
        p(wst_poswav, "Rectifiers");
        p(wst_negwav, "Rectifiers");
        p(wst_softrect, "Rectifiers");

        p(wst_softfold, "Wavefolder");
        p(wst_singlefold, "Wavefolder");
        p(wst_dualfold, "Wavefolder");
        p(wst_westfold, "Wavefolder");

        p(wst_fuzz, "Fuzz");
        p(wst_fuzzheavy, "Fuzz");
        p(wst_fuzzctr, "Fuzz");
        p(wst_fuzzsoft, "Fuzz");
        p(wst_fuzzsoftedge, "Fuzz");

        p(wst_sinpx, "Trigonometric");
        p(wst_sin2xpb, "Trigonometric");
        p(wst_sin3xpb, "Trigonometric");
        p(wst_sin7xpb, "Trigonometric");
        p(wst_sin10xpb, "Trigonometric");
        p(wst_2cyc, "Trigonometric");
        p(wst_7cyc, "Trigonometric");
        p(wst_10cyc, "Trigonometric");
        p(wst_2cycbound, "Trigonometric");
        p(wst_7cycbound, "Trigonometric");
        p(wst_10cycbound, "Trigonometric");

        int c = 0;
        for (auto e : mapping)
            inverseMapping[e.first] = c++;

        if (mapping.size() != n_ws_types)
            std::cout << "BAD MAPPING TYPES" << std::endl;
    }

    int remapStreamedIndexToDisplayIndex(int i) const override { return inverseMapping.at(i); }
    std::string nameAtStreamedIndex(int i) const override { return wst_names[i]; }
    bool hasGroupNames() const override { return true; };

    std::string groupNameAtStreamedIndex(int i) const override
    {
        return mapping[inverseMapping.at(i)].second;
    }

    bool sortGroupNames() const override { return false; }
    bool useRemappedOrderingForGroupsIfNotSorted() const override { return true; }

    bool supportsTotalIndexOrdering() const override { return true; }
    const std::vector<int> totalIndexOrdering() const override
    {
        auto res = std::vector<int>();
        for (auto m : mapping)
            res.push_back(m.first);
        return res;
    }
};