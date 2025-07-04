#pragma once

typedef struct {
    const char* child;
    const char* parent;
}default_tag_pair;

#define ARTIST_LINK(child, parent) { "artist:" child, "artist:" parent }
#define HIPHOP_ARTIST(str) { "artist:" str, "hip hop" }

constexpr default_tag_pair default_tag_parents[] = {
// Genres
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
    // TODO: Once we do proper escaping or parameterized queries, unncomment this.
    // HIPHOP_ARTIST("royce da 5'9\""),
    // HIPHOP_ARTIST("ol' dirty bastard"),
    HIPHOP_ARTIST("rza"),
    HIPHOP_ARTIST("gza"),
    HIPHOP_ARTIST("method man"),
    HIPHOP_ARTIST("raekwon"),
    HIPHOP_ARTIST("ghostface killah"),
    HIPHOP_ARTIST("u-god"),
    HIPHOP_ARTIST("inspectah deck"),
    HIPHOP_ARTIST("masta killa"),

// Artist aliases/groups
    ARTIST_LINK("yasiin bey", "mos def"),
    ARTIST_LINK("black star", "mos def"),
    ARTIST_LINK("black star", "talib kweli"),
    ARTIST_LINK("reflection eternal", "talib kweli"),
    ARTIST_LINK("reflection eternal", "dj hi-tek"),

    // Wu Tang Clan aliases
    ARTIST_LINK("rza", "prince rakeem"),
    ARTIST_LINK("rza", "RZArecta"),
    ARTIST_LINK("rza", "chief abbot"),
    ARTIST_LINK("rza", "bobby steels"),
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
    // ARTIST_LINK("wu tang clan",  "ol' dirty bastard"),

    ARTIST_LINK("the roots",  "black thought"),

    ARTIST_LINK("asheru and blue black of the unspoken heard", "asheru"),
    ARTIST_LINK("asheru and blue black of the unspoken heard", "blue black"),

    ARTIST_LINK("jurassic 5", "chali 2na"),
    ARTIST_LINK("public enemy", "chuck d"),

    ARTIST_LINK("onemanarmy", "one be lo"),
    ARTIST_LINK("binary star", "one be lo"),
    ARTIST_LINK("binary star", "senim silla"),

};
