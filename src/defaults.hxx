#pragma once

typedef struct {
    const char* child;
    const char* parent;
}default_tag_pair;

#define ARTIST_LINK(child, parent) { "artist:" child, "artist:" parent }
#define HIPHOP_ARTIST(str) { "artist:" str, "hip hop" }
#define IRISH_ARTIST(str) { "artist:" str, "irish" }
#define DNB_ARTIST(str) { "artist:" str, "drum and bass" }
#define HYPERPOP_ARTIST(str) { "artist:" str, "hyperpop" }
#define ELECTRONIC_ARTIST(str) { "artist:" str, "electronic" }
#define PUNK_ARTIST(str) { "artist:" str, "punk" }

static constexpr default_tag_pair default_tag_parents[] = {
// Genre parents
    {"drum and bass", "electronic"},
    {"dubstep", "electronic"},
    {"hyperpop", "electronic"},
    {"classic rock", "rock"},
    {"punk", "rock"},
    {"ska", "punk"},
    {"metal", "rock"},

// Genres
    {"less than jake", "ska"},
    {"skanatra", "ska"},
    {"streetlight manifesto", "ska"},
    PUNK_ARTIST("jerry big's world famous band"),
    PUNK_ARTIST("leaking head"),
    PUNK_ARTIST("street hassle"),
    PUNK_ARTIST("bikini kill"),
    PUNK_ARTIST("goldfinger"),
    PUNK_ARTIST("the presidents of the united states of america"),
    PUNK_ARTIST("the frumpies"),
    PUNK_ARTIST("sum 41"),
    PUNK_ARTIST("the clash"),
    PUNK_ARTIST("l7"),
    PUNK_ARTIST("adolescents"),
    PUNK_ARTIST("sonic youth"),
    PUNK_ARTIST("propagandhi"),
    PUNK_ARTIST("pinkshift"),
    PUNK_ARTIST("paramore"),
    {"paramore", "metal"},
    {"rainbow", "metal"},

    HIPHOP_ARTIST("mos def"),
    HIPHOP_ARTIST("talib kweli"),
    HIPHOP_ARTIST("a tribe called quest"),
    HIPHOP_ARTIST("public enemy"),
    HIPHOP_ARTIST("krs-one"),
    HIPHOP_ARTIST("gang starr"),
    HIPHOP_ARTIST("boogie down productions"),
    HIPHOP_ARTIST("beastie boys"),
    HIPHOP_ARTIST("q-tip"),
    HIPHOP_ARTIST("jarv"),
    HIPHOP_ARTIST("asheru"),
    HIPHOP_ARTIST("blue black"),
    HIPHOP_ARTIST("black thought"),
    HIPHOP_ARTIST("big pun"),
    HIPHOP_ARTIST("jurassic 5"),
    HIPHOP_ARTIST("chali 2na"),
    HIPHOP_ARTIST("ugly duckling"),
    HIPHOP_ARTIST("artofficial"),
    HIPHOP_ARTIST("senim silla"),
    HIPHOP_ARTIST("one be lo"),
    HIPHOP_ARTIST("de la soul"),
    HIPHOP_ARTIST("del the funky homosapien"),
    HIPHOP_ARTIST("busta rhymes"),
    HIPHOP_ARTIST("rakim"),
    HIPHOP_ARTIST("chuck d"),
    HIPHOP_ARTIST("grandmaster caz"),
    HIPHOP_ARTIST("young mc"),
    HIPHOP_ARTIST("nas"),
    HIPHOP_ARTIST("royce da 5'9\""),
    HIPHOP_ARTIST("ol' dirty bastard"),
    HIPHOP_ARTIST("rza"),
    HIPHOP_ARTIST("gza"),
    HIPHOP_ARTIST("method man"),
    HIPHOP_ARTIST("raekwon"),
    HIPHOP_ARTIST("ghostface killah"),
    HIPHOP_ARTIST("u-god"),
    HIPHOP_ARTIST("inspectah deck"),
    HIPHOP_ARTIST("masta killa"),
    HIPHOP_ARTIST("kendrick lamar"),
    HIPHOP_ARTIST("mf doom"), // Sorry to butcher the MF DOOM name like this. System works with lowercase - torph
    HIPHOP_ARTIST("macklemore"),
    HIPHOP_ARTIST("e-dubble"),

    IRISH_ARTIST("flogging molly"),
    IRISH_ARTIST("1916"),
    IRISH_ARTIST("dropkick murphys"),

    DNB_ARTIST("frequency"),
    DNB_ARTIST("glitchrode"),
    DNB_ARTIST("kettleonwater"),
    DNB_ARTIST("kubazx"),
    DNB_ARTIST("eightiesheadachetape"),

    HYPERPOP_ARTIST("golemm"),
    HYPERPOP_ARTIST("marshall4"),
    HYPERPOP_ARTIST("food house"),
    HYPERPOP_ARTIST("gupi"),
    HYPERPOP_ARTIST("fraxiom"),
    HYPERPOP_ARTIST("underscores"),
    HYPERPOP_ARTIST("cmten"),
    HYPERPOP_ARTIST("glitch gum"),
    HYPERPOP_ARTIST("100 gecs"),
    HYPERPOP_ARTIST("frost children"),
    HYPERPOP_ARTIST("saoirse dream"),
    HYPERPOP_ARTIST("jane remover"),
    HYPERPOP_ARTIST("webcage"),
    HYPERPOP_ARTIST("laura les"),

    ELECTRONIC_ARTIST("federation"),
    ELECTRONIC_ARTIST("home"),
    ELECTRONIC_ARTIST("no mana"),
    ELECTRONIC_ARTIST("ascension"),
    ELECTRONIC_ARTIST("toriena"),
    ELECTRONIC_ARTIST("adolf nomura"),
    ELECTRONIC_ARTIST("paul oakenfold"),
    ELECTRONIC_ARTIST("morch kovalski"),
    ELECTRONIC_ARTIST("holy fuck"),
    ELECTRONIC_ARTIST("emancipator"),
    ELECTRONIC_ARTIST("danger"),
    ELECTRONIC_ARTIST("waveshaper"),
    ELECTRONIC_ARTIST("the toxic avenger"),
    ELECTRONIC_ARTIST("scattle"),
    {"album:furi (original game soundtrack)", "electronic"},
    {"album:pinout (original soundtrack)", "electronic"},
    ELECTRONIC_ARTIST("fred v & grafx"),
    ELECTRONIC_ARTIST("hooverphonic"),
    ELECTRONIC_ARTIST("flashygoodness"),
    ELECTRONIC_ARTIST("chemical brothers"),
    ELECTRONIC_ARTIST("bachelors of science"),
    ELECTRONIC_ARTIST("new order"),
    ELECTRONIC_ARTIST("gereon"),
    {"skrillex", "dubstep"},
    ELECTRONIC_ARTIST("meganeko"),
    ELECTRONIC_ARTIST("ex-lyd"),
    ELECTRONIC_ARTIST("power glove"),
    ELECTRONIC_ARTIST("bignic"),

// Artist aliases/groups
    ARTIST_LINK("yasiin bey", "mos def"),
    ARTIST_LINK("black star", "mos def"),
    ARTIST_LINK("black star", "talib kweli"),
    ARTIST_LINK("reflection eternal", "talib kweli"),
    ARTIST_LINK("reflection eternal", "dj hi-tek"),

    // Wu Tang Clan aliases
    ARTIST_LINK("raekwon the chef", "raekwon"),
    ARTIST_LINK("wu tang clan", "rza"),
    ARTIST_LINK("wu tang clan", "gza"),
    ARTIST_LINK("wu tang clan", "method man"),
    ARTIST_LINK("wu tang clan", "raekwon"),
    ARTIST_LINK("wu tang clan", "ghostface killah"),
    ARTIST_LINK("wu tang clan", "u-god"),
    ARTIST_LINK("wu tang clan", "inspectah deck"),
    ARTIST_LINK("wu tang clan", "masta killa"),
    // TODO: Once we do proper escaping or parameterized queries, unncomment this.
    ARTIST_LINK("wu tang clan",  "ol' dirty bastard"),

    ARTIST_LINK("the roots",  "black thought"),

    ARTIST_LINK("asheru and blue black of the unspoken heard", "asheru"),
    ARTIST_LINK("asheru and blue black of the unspoken heard", "blue black"),

    ARTIST_LINK("jurassic 5", "chali 2na"),
    ARTIST_LINK("public enemy", "chuck d"),

    ARTIST_LINK("onemanarmy", "one be lo"),
    ARTIST_LINK("binary star", "one be lo"),
    ARTIST_LINK("binary star", "senim silla"),

};
