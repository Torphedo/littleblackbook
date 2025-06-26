#include "cli_main.hxx"
#include <cstdio>

#include <common/logging.h>
#include <common/path.h>
#include <cstdlib>

#include <song.hxx>
#include <tags.hxx>
#include <sqlgen.hxx>
#include <schema.hxx>

static const char* version_string = "1.0.0";
static const char* url = "https://github.com/Torphedo";

int cli_main(const arguments& args, sqlite3* db, const char* db_path, const std::string& db_files_folder) {
    // Parse arguments
    const char* flag = args.argv[1];

    if (args.settings[SETTING_ARG_HELP]) {
        printf("[help message not written yet]\n");
    } else if (args.settings[SETTING_ARG_VERSION]) {
        printf("%s v%s [Open source @ %s]", args.argv[0], version_string, url);
        printf("Written by Torphedo\n");
    }

    // There's more args that aren't settings flags...
    // Treat them as filenames.

    if (args.settings[SETTING_ARG_IMPORT]) {
        const u32 num_files = args.argc - args.first_non_flag;
        const char* const* files = &args.argv[args.first_non_flag];
        import_stats_t stats;
        import_many_files_many_threads(files, num_files, db_files_folder.c_str(), db, &stats);
    }

    if (args.settings[SETTING_ARG_SEARCH]) {
        std::string sqlbuf;
        const u32 num_tags = args.argc - args.first_non_flag;
        const char* const* tags = &args.argv[args.first_non_flag];
        search_many_tags_and(tags, num_tags, sqlbuf);

        // Dump generated SQL to a file
        const char* sqlpath = "query.sql";
        FILE* f = fopen(sqlpath, "wb");
        if (f) {
            fprintf(f, "%s\n", sqlbuf.c_str());
            fclose(f);
            LOG_MSG(info, "Saved search query to \"%s\"\n", sqlpath);
            LOG_MSG(info, "You can get the results by piping the file into the \"sqlite3\" utility [e.g. 'cat query.sql | sqlite3 file.db']\n", sqlpath);
        } else {
            LOG_MSG(error, "Failed to save search query to \"%s\"\n", sqlpath);
        }
    }

    if (args.seen_values[VALUE_ARG_NEW_TAG]) {
        const char* tag = args.values[VALUE_ARG_NEW_TAG];
        // Temporary hardcoded value, eventually should take this on command-line
        const song_hash_t song_hash = 347807049;
        std::string sqlbuf;
        add_tag_sql(tag, song_hash, sqlbuf);
        sqlgen_exec(db, sqlbuf.data());
    }

    // User wants to create a parent-child relationship between 2 tags
    if (args.settings[SETTING_ARG_LINK_TAGS]) {
        bool can_proceed = args.seen_values[VALUE_ARG_PARENT] && args.seen_values[VALUE_ARG_CHILD];
        if (args.seen_values[VALUE_ARG_PARENT]) {
            LOG_MSG(error, "You didn't provide a parent tag, so I don't know what to attach to the child.\n");
            can_proceed = false;
        }
        if (args.seen_values[VALUE_ARG_CHILD]) {
            LOG_MSG(error, "You didn't provide a child tag, so I don't know what to assign the parent to.\n");
            can_proceed = false;
        }

        if (can_proceed) {
            std::string sqlbuf;
            const char* parent = args.values[VALUE_ARG_PARENT];
            const char* child = args.values[VALUE_ARG_CHILD];
            link_tags_sql(parent, child, sqlbuf);
            sqlgen_exec(db, sqlbuf.data());
        }
    }

    return EXIT_SUCCESS;
}
