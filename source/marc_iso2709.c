#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>

#include <viiewlib/marc.h>

#include "marc_encoder.h"

#define MAX_GAMES 100
#define MAX_SEARCH_RESULTS 100
#define MAX_IMPORTED_RECORDS 100
#define IMPORTED_RECORD_FILENAME_LENGTH 64
#define IMPORTED_RECORD_TITLE_LENGTH 100
#define IMPORTED_RECORD_ID_LENGTH 7
#define IMPORTED_RECORD_001_LENGTH 100

#define DATABASE_LINE_LENGTH 4096
#define MAX_INFO_LINES 100
#define INFO_LINE_LENGTH 100

#define MAX_MARC_LINES 300
#define MARC_LINE_LENGTH 100

#define SEARCH_LENGTH 100

#define CONSOLE_COLUMNS 80
#define UI_WIDTH 50


static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static int menu_selection = 0;
static int screen = 0;

static int catalogue_selection = 0;
static int game_count = 0;

static int info_scroll = 0;

static int marc_selection = 0;
static int marc_scroll = 0;
static int marc_line_count = 0;

static int search_selection = 0;
static int search_result_count = 0;

static char search_query[SEARCH_LENGTH];
static int search_length = 0;


#define SEARCH_MODE_NORMAL 0
#define SEARCH_MODE_MARC21 1

static int search_mode = SEARCH_MODE_NORMAL;
static int search_mode_selection = 0;


/*
    Tracks how the current MARC record screen was opened.

    0 = opened from the main MARC Records menu
    1 = opened from MARC21 search results
*/
static int marc_record_from_search = 0;


/*
    Settings screens.

    0 = Reload SD / USB
    1 = Reload Databases
    2 = Credits
*/
static int settings_selection = 0;


/*
    Stores the result of the most recent settings
    operation so the settings screen can display
    a useful status message.
*/
static char settings_status[160] = "";

void show_encode_marc(void);

/*
    Encode MARC screens.

    Screen 11 = Encode MARC menu
    Screen 12 = Select game for MARC export
*/
static int encode_selection = 0;
static int encode_game_selection = 0;

static char encode_status[160] = "";


#define INPUT_UP       (1 << 0)
#define INPUT_DOWN     (1 << 1)
#define INPUT_LEFT     (1 << 2)
#define INPUT_RIGHT    (1 << 3)
#define INPUT_SELECT   (1 << 4)
#define INPUT_BACK     (1 << 5)
#define INPUT_PLUS     (1 << 6)
#define INPUT_HOME     (1 << 7)


void print_spaces(
    int count
) {

    for (
        int i = 0;
        i < count;
        i++
    )
        putchar(' ');
}


void print_centered(
    const char *text
) {

    int length =
        strlen(text);

    int padding =
        (CONSOLE_COLUMNS - length) / 2;

    if (
        padding < 0
    )
        padding = 0;

    print_spaces(
        padding
    );

    printf(
        "%s\n",
        text
    );
}


void print_ui_line(
    char character
) {

    int padding =
        (CONSOLE_COLUMNS - UI_WIDTH) / 2;

    print_spaces(
        padding
    );

    for (
        int i = 0;
        i < UI_WIDTH;
        i++
    )
        putchar(character);

    putchar('\n');
}


void print_menu_item(
    const char *text,
    int selected
) {

    char line[160];

    if (
        selected
    ) {

        snprintf(
            line,
            sizeof(line),
            "> %s",
            text
        );

    } else {

        snprintf(
            line,
            sizeof(line),
            "  %s",
            text
        );
    }

    print_centered(
        line
    );
}


void print_label_value(
    const char *label,
    const char *value
) {

    char line[160];

    snprintf(
        line,
        sizeof(line),
        "%s%s",
        label,
        value
    );

    print_centered(
        line
    );
}


u32 get_input() {

    u32 pressed =
        WPAD_ButtonsDown(0);

    u32 input = 0;


    if (
        pressed & WPAD_BUTTON_UP
    )
        input |= INPUT_UP;

    if (
        pressed & WPAD_CLASSIC_BUTTON_UP
    )
        input |= INPUT_UP;


    if (
        pressed & WPAD_BUTTON_DOWN
    )
        input |= INPUT_DOWN;

    if (
        pressed & WPAD_CLASSIC_BUTTON_DOWN
    )
        input |= INPUT_DOWN;


    if (
        pressed & WPAD_BUTTON_LEFT
    )
        input |= INPUT_LEFT;

    if (
        pressed & WPAD_CLASSIC_BUTTON_LEFT
    )
        input |= INPUT_LEFT;


    if (
        pressed & WPAD_BUTTON_RIGHT
    )
        input |= INPUT_RIGHT;

    if (
        pressed & WPAD_CLASSIC_BUTTON_RIGHT
    )
        input |= INPUT_RIGHT;


    if (
        pressed & WPAD_BUTTON_A
    )
        input |= INPUT_SELECT;

    if (
        pressed & WPAD_CLASSIC_BUTTON_A
    )
        input |= INPUT_SELECT;


    if (
        pressed & WPAD_BUTTON_B
    )
        input |= INPUT_BACK;

    if (
        pressed & WPAD_CLASSIC_BUTTON_B
    )
        input |= INPUT_BACK;


    if (
        pressed & WPAD_BUTTON_PLUS
    )
        input |= INPUT_PLUS;

    if (
        pressed & WPAD_CLASSIC_BUTTON_PLUS
    )
        input |= INPUT_PLUS;


    if (
        pressed & WPAD_BUTTON_HOME
    )
        input |= INPUT_HOME;


    if (
        pressed & WPAD_CLASSIC_BUTTON_HOME
    )
        input |= INPUT_HOME;


    return input;
}


#define KEYBOARD_ROWS 4
#define KEYBOARD_COLUMNS 10

static const char keyboard[KEYBOARD_ROWS][KEYBOARD_COLUMNS + 1] = {
    "ABCDEFGHIJ",
    "KLMNOPQRST",
    "UVWXYZ0123",
    "456789"
};

static int keyboard_row = 0;
static int keyboard_column = 0;
static int keyboard_special_selection = 0;


typedef struct {

    char title[100];
    char id[7];
    char platform[30];
    char region[20];
    char release_date[30];
    char publisher[50];
    char developer[50];
    char genre[50];
    char series[50];
    char synopsis[3000];

} Game;


static Game games[MAX_GAMES];

typedef struct { char filename[IMPORTED_RECORD_FILENAME_LENGTH]; char title[IMPORTED_RECORD_TITLE_LENGTH]; char game_id[IMPORTED_RECORD_ID_LENGTH]; char marc_001[IMPORTED_RECORD_001_LENGTH]; } ImportedRecord;
static ImportedRecord imported_records[MAX_IMPORTED_RECORDS];
static int imported_record_count = 0;
static int imported_selection = 0;
static int imported_scroll = 0;


static int search_results[MAX_SEARCH_RESULTS];


static char marc_lines[
    MAX_MARC_LINES
][
    MARC_LINE_LENGTH
];


void extract_game_id(
    const char *name,
    char *id
) {

    const char *start =
        strchr(name, '[');

    if (
        start != NULL &&
        strlen(start) >= 8
    ) {

        strncpy(
            id,
            start + 1,
            6
        );

        id[6] = '\0';

    } else {

        strcpy(
            id,
            "??????"
        );
    }
}


void extract_game_title(
    const char *name,
    char *title
) {

    const char *start =
        strchr(name, '[');

    if (
        start != NULL
    ) {

        int length =
            start - name;

        if (
            length >= 100
        )
            length = 99;

        strncpy(
            title,
            name,
            length
        );

        title[length] = '\0';

        while (
            length > 0 &&
            title[length - 1] == ' '
        ) {

            title[length - 1] =
                '\0';

            length--;
        }

    } else {

        strncpy(
            title,
            name,
            99
        );

        title[99] = '\0';
    }
}


void setup_game_metadata(
    Game *game
) {

    strcpy(
        game->platform,
        "Nintendo Wii"
    );

    strcpy(
        game->region,
        "Unknown"
    );

    strcpy(
        game->release_date,
        "Not yet catalogued"
    );

    strcpy(
        game->publisher,
        "Not yet catalogued"
    );

    strcpy(
        game->developer,
        "Not yet catalogued"
    );

    strcpy(
        game->genre,
        "Not yet catalogued"
    );

    strcpy(
        game->series,
        "Not yet catalogued"
    );

    strcpy(
        game->synopsis,
        "Not yet catalogued"
    );
}


void copy_database_value(
    const char *line,
    const char *field,
    char *destination,
    int destination_size
) {

    int field_length =
        strlen(field);

    if (
        strncmp(
            line,
            field,
            field_length
        ) == 0
    ) {

        int value_length =
            strlen(line + field_length);

        if (
            value_length >=
            destination_size
        )
            value_length =
                destination_size - 1;

        memcpy(
            destination,
            line + field_length,
            value_length
        );

        destination[value_length] =
            '\0';
    }
}



static int imported_get_id(const char *filename, char *id) {
    const char *p="marcviiew_"; const char *dot;
    size_t n;
    if (!filename || !id || strncmp(filename,p,10)!=0) return 0;
    dot=strrchr(filename,'.'); if(!dot) return 0;
    n=(size_t)(dot-(filename+10)); if(n!=6) return 0;
    memcpy(id,filename+10,6); id[6]='\\0'; return 1;
}
static int imported_subfield(MARC_Field *f,char code,char *out,size_t size) {
    size_t i,n; MARC_Subfield *s; const char *v;
    if(!f||!out||!size) return 0; n=marc_field_get_subfield_count(f);
    for(i=0;i<n;i++){ s=marc_field_get_subfield(f,i); if(s&&marc_subfield_get_code(s)==code){v=marc_subfield_get_value(s); if(!v)return 0; strncpy(out,v,size-1);out[size-1]='\\0';return 1;}}
    return 0;
}
static int imported_load(const char *path,const char *filename,ImportedRecord *out) {
    FILE *f; MARC_Record *r; MARC_Field *field; if(!path||!filename||!out)return 0;
    memset(out,0,sizeof(*out)); if(!imported_get_id(filename,out->game_id))return 0;
    strncpy(out->filename,filename,sizeof(out->filename)-1); f=fopen(path,"rb"); if(!f)return 0;
    r=marc_record_create(); if(!r){fclose(f);return 0;} if(marc_record_read(r,f)!=0){marc_record_free(r);fclose(f);return 0;} fclose(f);
    field=marc_record_get_field_by_tag(r,"001"); if(field){const char *v=marc_field_get_control_value(field); if(v)strncpy(out->marc_001,v,sizeof(out->marc_001)-1);}
    field=marc_record_get_field_by_tag(r,"245"); if(field)imported_subfield(field,'a',out->title,sizeof(out->title));
    if(!out->title[0])strcpy(out->title,"(Untitled MARC record)"); if(!out->marc_001[0])strcpy(out->marc_001,"(none)"); marc_record_free(r); return 1;
}
static int imported_exists(const ImportedRecord *x) {
    int i; for(i=0;i<imported_record_count;i++) if(!strcmp(imported_records[i].filename,x->filename)||!strcmp(imported_records[i].game_id,x->game_id))return 1; return 0;
}
static void imported_add(const ImportedRecord *x) { if(x&&imported_record_count<MAX_IMPORTED_RECORDS) imported_records[imported_record_count++]=*x; }
static void imported_scan_dir(const char *dirpath) {
    DIR *d; struct dirent *e; char path[256]; ImportedRecord x;
    d=opendir(dirpath); if(!d)return;
    while((e=readdir(d))!=NULL){ if(e->d_name[0]=='.'||imported_record_count>=MAX_IMPORTED_RECORDS)continue; if(!strrchr(e->d_name,'.'))continue; {const char *ext=strrchr(e->d_name,'.'); if(strlen(ext)!=4||tolower((unsigned char)ext[1])!='m'||tolower((unsigned char)ext[2])!='r'||tolower((unsigned char)ext[3])!='c')continue;}
        snprintf(path,sizeof(path),"%s/%s",dirpath,e->d_name); if(imported_load(path,e->d_name,&x)&&!imported_exists(&x))imported_add(&x);
    } closedir(d);
}
static void scan_imported_records(void) {
    DIR *d; struct dirent *e; char src[256],dst[256]; ImportedRecord x;
    imported_record_count=0; imported_selection=0; imported_scroll=0;
    mkdir("sd:/marcviiew_import",0777); mkdir("sd:/marcviiew",0777); mkdir("sd:/marcviiew/imported",0777);
    imported_scan_dir("sd:/marcviiew/imported"); d=opendir("sd:/marcviiew_import"); if(!d)return;
    while((e=readdir(d))!=NULL){ if(e->d_name[0]=='.'||imported_record_count>=MAX_IMPORTED_RECORDS)continue; snprintf(src,sizeof(src),"sd:/marcviiew_import/%s",e->d_name); if(!imported_load(src,e->d_name,&x)||imported_exists(&x))continue; snprintf(dst,sizeof(dst),"sd:/marcviiew/imported/%s",e->d_name); if(rename(src,dst)==0)imported_add(&x); }
    closedir(d);
}

int find_game_by_id(
    const char *id
) {

    for (
        int i = 0;
        i < game_count;
        i++
    ) {

        if (
            strcmp(
                games[i].id,
                id
            ) == 0
        )
            return i;
    }

    return -1;
}


int load_game_database() {

    FILE *database;

    char line[
        DATABASE_LINE_LENGTH
    ];

    int current_game_index = -1;

    database =
        fopen(
            "sd:/marcviiew_games.txt",
            "r"
        );

    if (
        database == NULL
    )
        return 0;

    while (
        fgets(
            line,
            sizeof(line),
            database
        ) != NULL
    ) {

        line[
            strcspn(
                line,
                "\r\n"
            )
        ] = '\0';


        if (
            strncmp(
                line,
                "ID=",
                3
            ) == 0
        ) {

            current_game_index =
                find_game_by_id(
                    line + 3
                );

            continue;
        }


        if (
            current_game_index < 0
        )
            continue;


        if (
            strlen(line) == 0
        ) {

            current_game_index = -1;

            continue;
        }


        copy_database_value(
            line,
            "TITLE=",
            games[
                current_game_index
            ].title,
            sizeof(
                games[
                    current_game_index
                ].title
            )
        );

        copy_database_value(
            line,
            "REGION=",
            games[
                current_game_index
            ].region,
            sizeof(
                games[
                    current_game_index
                ].region
            )
        );

        copy_database_value(
            line,
            "DEVELOPER=",
            games[
                current_game_index
            ].developer,
            sizeof(
                games[
                    current_game_index
                ].developer
            )
        );

        copy_database_value(
            line,
            "PUBLISHER=",
            games[
                current_game_index
            ].publisher,
            sizeof(
                games[
                    current_game_index
                ].publisher
            )
        );

        copy_database_value(
            line,
            "RELEASE_DATE=",
            games[
                current_game_index
            ].release_date,
            sizeof(
                games[
                    current_game_index
                ].release_date
            )
        );

        copy_database_value(
            line,
            "GENRE=",
            games[
                current_game_index
            ].genre,
            sizeof(
                games[
                    current_game_index
                ].genre
            )
        );

        copy_database_value(
            line,
            "SYNOPSIS=",
            games[
                current_game_index
            ].synopsis,
            sizeof(
                games[
                    current_game_index
                ].synopsis
            )
        );
    }

    fclose(
        database
    );

    return 1;
}


void scan_storage(
    const char *storage_path
) {

    DIR *dir;

    struct dirent *entry;

    dir =
        opendir(
            storage_path
        );

    if (
        dir == NULL
    )
        return;


    while (
        (entry = readdir(dir))
        != NULL
    ) {

        if (
            entry->d_name[0] == '.'
        )
            continue;

        if (
            game_count >=
            MAX_GAMES
        )
            break;


        extract_game_title(
            entry->d_name,
            games[
                game_count
            ].title
        );

        extract_game_id(
            entry->d_name,
            games[
                game_count
            ].id
        );

        setup_game_metadata(
            &games[
                game_count
            ]
        );

        game_count++;
    }

    closedir(
        dir
    );
}


void scan_catalogue() {

    game_count = 0;

    scan_storage(
        "sd:/wbfs"
    );

    if (
        game_count <
        MAX_GAMES
    ) {

        scan_storage(
            "usb:/wbfs"
        );
    }

    load_game_database();
}


int reload_storage() {

    int result;

    fatUnmount(
        "sd"
    );

    fatUnmount(
        "usb"
    );

    result =
        fatInitDefault();

    return result;
}


int reload_databases() {

    FILE *game_database;
    FILE *marc_database;

    int game_database_loaded = 0;
    int marc_database_loaded = 0;

    search_result_count = 0;
    search_selection = 0;

    search_length = 0;
    search_query[0] = '\0';

    marc_record_from_search = 0;

    catalogue_selection = 0;
    marc_selection = 0;

    info_scroll = 0;
    marc_scroll = 0;

    encode_selection = 0;
    encode_game_selection = 0;
    encode_status[0] = '\0';

    scan_catalogue();
    scan_imported_records();

    /*
        The normal MarcViiew LMS uses the text databases.
        The .mrc files are exports only.
    */
    game_database =
        fopen(
            "sd:/marcviiew_games.txt",
            "r"
        );

    if (
        game_database != NULL
    ) {

        game_database_loaded = 1;

        fclose(
            game_database
        );
    }


    marc_database =
        fopen(
            "sd:/marcviiew_marc.txt",
            "r"
        );

    if (
        marc_database != NULL
    ) {

        marc_database_loaded = 1;

        fclose(
            marc_database
        );
    }


    if (
        game_database_loaded &&
        marc_database_loaded
    ) {

        snprintf(
            settings_status,
            sizeof(settings_status),
            "Both databases reloaded. %d game(s) found.",
            game_count
        );

    }

    else if (
        game_database_loaded
    ) {

        snprintf(
            settings_status,
            sizeof(settings_status),
            "Game database OK; MARC database not found."
        );

    }

    else if (
        marc_database_loaded
    ) {

        snprintf(
            settings_status,
            sizeof(settings_status),
            "MARC database OK; game database not found."
        );

    }

    else {

        snprintf(
            settings_status,
            sizeof(settings_status),
            "Neither database was found."
        );
    }

    return (
        game_database_loaded &&
        marc_database_loaded
    );
}


void add_info_line(
    char info_lines[][INFO_LINE_LENGTH],
    int *line_count,
    const char *text
) {

    if (
        *line_count >=
        MAX_INFO_LINES
    )
        return;

    strncpy(
        info_lines[*line_count],
        text,
        INFO_LINE_LENGTH - 1
    );

    info_lines[
        *line_count
    ][
        INFO_LINE_LENGTH - 1
    ] = '\0';

    (*line_count)++;
}


void add_info_field(
    char info_lines[][INFO_LINE_LENGTH],
    int *line_count,
    const char *label,
    const char *value
) {

    char line[
        INFO_LINE_LENGTH
    ];

    snprintf(
        line,
        sizeof(line),
        "%s:",
        label
    );

    add_info_line(
        info_lines,
        line_count,
        line
    );

    add_info_line(
        info_lines,
        line_count,
        value
    );

    add_info_line(
        info_lines,
        line_count,
        ""
    );
}


void add_wrapped_text(
    char info_lines[][INFO_LINE_LENGTH],
    int *line_count,
    const char *text
) {

    const int wrap_width = 70;

    int length =
        strlen(text);

    int position = 0;

    while (
        position < length &&
        *line_count <
        MAX_INFO_LINES
    ) {

        int remaining =
            length - position;

        int line_length =
            remaining;

        if (
            line_length >
            wrap_width
        )
            line_length =
                wrap_width;

        if (
            position + line_length <
            length
        ) {

            int break_position =
                line_length;

            while (
                break_position > 0 &&
                text[
                    position +
                    break_position
                ] != ' '
            )
                break_position--;

            if (
                break_position > 0
            )
                line_length =
                    break_position;
        }

        strncpy(
            info_lines[
                *line_count
            ],
            text + position,
            line_length
        );

        info_lines[
            *line_count
        ][
            line_length
        ] = '\0';

        (*line_count)++;

        position += line_length;

        while (
            position < length &&
            text[position] == ' '
        )
            position++;
    }
}


void show_game_information() {

    char info_lines[
        MAX_INFO_LINES
    ][
        INFO_LINE_LENGTH
    ];

    int line_count = 0;

    Game *game =
        &games[
            catalogue_selection
        ];


    add_info_field(
        info_lines,
        &line_count,
        "Title",
        game->title
    );

    add_info_field(
        info_lines,
        &line_count,
        "Game ID",
        game->id
    );

    add_info_field(
        info_lines,
        &line_count,
        "Platform",
        game->platform
    );

    add_info_field(
        info_lines,
        &line_count,
        "Region",
        game->region
    );

    add_info_field(
        info_lines,
        &line_count,
        "Release Date",
        game->release_date
    );

    add_info_field(
        info_lines,
        &line_count,
        "Publisher",
        game->publisher
    );

    add_info_field(
        info_lines,
        &line_count,
        "Developer",
        game->developer
    );

    add_info_field(
        info_lines,
        &line_count,
        "Genre",
        game->genre
    );

    add_info_field(
        info_lines,
        &line_count,
        "Series",
        game->series
    );

    add_info_line(
        info_lines,
        &line_count,
        "Synopsis:"
    );

    add_wrapped_text(
        info_lines,
        &line_count,
        game->synopsis
    );

    add_info_line(
        info_lines,
        &line_count,
        ""
    );


    printf(
        "\x1b[2J\x1b[H"
    );

    print_ui_line('=');

    print_centered(
        "Game Information"
    );

    print_ui_line('=');

    printf("\n");


    int visible_lines = 17;

    int max_scroll =
        line_count -
        visible_lines;

    if (
        max_scroll < 0
    )
        max_scroll = 0;

    if (
        info_scroll >
        max_scroll
    )
        info_scroll =
            max_scroll;

    if (
        info_scroll < 0
    )
        info_scroll = 0;


    for (
        int i = info_scroll;
        i <
            info_scroll +
            visible_lines &&
        i < line_count;
        i++
    ) {

        print_centered(
            info_lines[i]
        );
    }


    printf("\n");


    if (
        max_scroll > 0
    ) {

        char scroll_line[80];

        snprintf(
            scroll_line,
            sizeof(scroll_line),
            "UP / DOWN = Scroll  (%d/%d)",
            info_scroll + 1,
            max_scroll + 1
        );

        print_centered(
            scroll_line
        );

    } else {

        print_centered(
            "UP / DOWN = Scroll"
        );
    }


    print_centered(
        "B = Back"
    );
}


void add_marc_line(
    const char *text
) {

    if (
        marc_line_count >=
        MAX_MARC_LINES
    )
        return;

    strncpy(
        marc_lines[
            marc_line_count
        ],
        text,
        MARC_LINE_LENGTH - 1
    );

    marc_lines[
        marc_line_count
    ][
        MARC_LINE_LENGTH - 1
    ] = '\0';

    marc_line_count++;
}


void add_marc_wrapped_subfield(
    const char *prefix,
    const char *text
) {

    const int wrap_width = 70;

    int text_length =
        strlen(text);

    int position = 0;

    int first_line = 1;


    while (
        position < text_length &&
        marc_line_count < MAX_MARC_LINES
    ) {

        int remaining =
            text_length - position;

        int available_width =
            wrap_width;

        if (
            first_line
        )
            available_width -= strlen(prefix);


        if (
            available_width < 1
        )
            available_width = 1;


        int line_length =
            remaining;

        if (
            line_length >
            available_width
        )
            line_length =
                available_width;


        if (
            position + line_length <
            text_length
        ) {

            int break_position =
                line_length;

            while (
                break_position > 0 &&
                text[
                    position +
                    break_position
                ] != ' '
            )
                break_position--;

            if (
                break_position > 0
            )
                line_length =
                    break_position;
        }


        char output_line[
            MARC_LINE_LENGTH
        ];


        if (
            first_line
        ) {

            snprintf(
                output_line,
                sizeof(output_line),
                "%s%.*s",
                prefix,
                line_length,
                text + position
            );

            first_line = 0;

        } else {

            snprintf(
                output_line,
                sizeof(output_line),
                "    %.*s",
                line_length,
                text + position
            );
        }


        add_marc_line(
            output_line
        );


        position += line_length;


        while (
            position < text_length &&
            text[position] == ' '
        )
            position++;
    }


    if (
        text_length == 0 &&
        marc_line_count < MAX_MARC_LINES
    ) {

        add_marc_line(
            prefix
        );
    }
}


/*
    Load a MARC record from the normal MarcViiew text
    database.

    IMPORTANT:

    This is intentionally NOT the ISO 2709 encoder.

    The LMS continues to use:

        sd:/marcviiew_marc.txt

    The .mrc files generated by ViiewLib are exports
    only.

    The selected Wii game's ID is matched against:

        [RECORD]
        GAME_ID=xxxxxx

    and the complete corresponding MARC record is
    rendered on the MARC viewer screen.
*/
int load_marc_record() {

    FILE *marc_file;

    char line[
        DATABASE_LINE_LENGTH
    ];

    char target_id[7];

    int in_record = 0;
    int matching_record = 0;

    int current_field = 0;


    marc_line_count = 0;


    if (
        game_count == 0
    )
        return 0;


    if (
        marc_selection < 0 ||
        marc_selection >= game_count
    )
        return 0;


    strcpy(
        target_id,
        games[
            marc_selection
        ].id
    );


    marc_file =
        fopen(
            "sd:/marcviiew_marc.txt",
            "r"
        );


    if (
        marc_file == NULL
    )
        return -1;


    while (
        fgets(
            line,
            sizeof(line),
            marc_file
        ) != NULL
    ) {

        line[
            strcspn(
                line,
                "\r\n"
            )
        ] = '\0';


        /*
            A new [RECORD] starts a new MARC record.
        */
        if (
            strcmp(
                line,
                "[RECORD]"
            ) == 0
        ) {

            in_record = 1;
            matching_record = 0;
            current_field = 0;

            continue;
        }


        if (
            !in_record
        )
            continue;


        /*
            GAME_ID identifies the application-level
            record corresponding to the Wii game.
        */
        if (
            strncmp(
                line,
                "GAME_ID=",
                8
            ) == 0
        ) {

            if (
                strcmp(
                    line + 8,
                    target_id
                ) == 0
            ) {

                matching_record = 1;

            } else {

                matching_record = 0;
            }

            continue;
        }


        if (
            !matching_record
        )
            continue;


        /*
            A blank line ends the current record.
        */
        if (
            strlen(line) == 0
        ) {

            if (
                matching_record
            ) {

                fclose(
                    marc_file
                );

                return 1;
            }

            in_record = 0;
            continue;
        }


        /*
            [001], [245], [264], etc.
        */
        if (
            line[0] == '['
        ) {

            char tag[4];

            if (
                strlen(line) >= 5 &&
                line[4] == ']'
            ) {

                tag[0] = line[1];
                tag[1] = line[2];
                tag[2] = line[3];
                tag[3] = '\0';

                /*
                    Control field.
                */
                if (
                    atoi(tag) < 10
                ) {

                    current_field = atoi(tag);

                } else {

                    /*
                        Data field header will be read
                        from IND1 / IND2 lines below.
                    */
                    current_field = atoi(tag);
                }
            }

            continue;
        }


        /*
            Control field:
                VALUE=007E01
        */
        if (
            current_field >= 0 &&
            current_field < 10 &&
            strncmp(
                line,
                "VALUE=",
                6
            ) == 0
        ) {

            char output[
                MARC_LINE_LENGTH
            ];

            snprintf(
                output,
                sizeof(output),
                "%03d %s",
                current_field,
                line + 6
            );

            add_marc_line(
                output
            );

            continue;
        }


        /*
            Data field indicators.

            The source format uses:

                IND1=#
                IND2=0

            Blank indicators are represented by '#'.
        */
        if (
            strncmp(
                line,
                "IND1=",
                5
            ) == 0
        ) {

            char indicator1 =
                line[5];

            char next_line[
                DATABASE_LINE_LENGTH
            ];

            long position =
                ftell(
                    marc_file
                );

            if (
                fgets(
                    next_line,
                    sizeof(next_line),
                    marc_file
                ) == NULL
            ) {

                fclose(
                    marc_file
                );

                return -1;
            }

            next_line[
                strcspn(
                    next_line,
                    "\r\n"
                )
            ] = '\0';


            if (
                strncmp(
                    next_line,
                    "IND2=",
                    5
                ) != 0
            ) {

                fclose(
                    marc_file
                );

                return -1;
            }


            char indicator2 =
                next_line[5];


            if (
                indicator1 == '#'
            )
                indicator1 = ' ';

            if (
                indicator2 == '#'
            )
                indicator2 = ' ';


            char header[
                MARC_LINE_LENGTH
            ];

            snprintf(
                header,
                sizeof(header),
                "%03d %c%c",
                current_field,
                indicator1,
                indicator2
            );

            add_marc_line(
                header
            );

            (void)position;

            continue;
        }


        /*
            Subfield:

                $a=value
                $b=value
                etc.

            The source database may contain leading
            whitespace before the $.
        */
        char *subfield_start =
            strchr(
                line,
                '$'
            );


        if (
            subfield_start != NULL &&
            subfield_start[1] != '\0' &&
            subfield_start[2] == '='
        ) {

            char prefix[16];

            snprintf(
                prefix,
                sizeof(prefix),
                "  $%c=",
                subfield_start[1]
            );


            const char *value =
                subfield_start + 3;


            if (
                current_field == 500
            ) {

                add_marc_wrapped_subfield(
                    prefix,
                    value
                );

            } else {

                char output[
                    MARC_LINE_LENGTH
                ];

                snprintf(
                    output,
                    sizeof(output),
                    "%s%s",
                    prefix,
                    value
                );

                add_marc_line(
                    output
                );
            }

            continue;
        }
    }


    fclose(
        marc_file
    );


    if (
        matching_record
    )
        return 1;


    return 0;
}


int contains_ignore_case(
    const char *haystack,
    const char *needle
) {

    int haystack_length =
        strlen(haystack);

    int needle_length =
        strlen(needle);

    if (
        needle_length == 0
    )
        return 1;

    if (
        needle_length >
        haystack_length
    )
        return 0;


    for (
        int i = 0;
        i <=
            haystack_length -
            needle_length;
        i++
    ) {

        int match = 1;


        for (
            int j = 0;
            j < needle_length;
            j++
        ) {

            char a =
                tolower(
                    (unsigned char)
                    haystack[i + j]
                );

            char b =
                tolower(
                    (unsigned char)
                    needle[j]
                );


            if (
                a != b
            ) {

                match = 0;

                break;
            }
        }


        if (
            match
        )
            return 1;
    }


    return 0;
}


void perform_marc_search() {

    FILE *marc_file;

    char line[
        DATABASE_LINE_LENGTH
    ];

    char current_game_id[7] = "";

    int in_record = 0;

    int record_matches = 0;


    search_result_count = 0;


    marc_file =
        fopen(
            "sd:/marcviiew_marc.txt",
            "r"
        );


    if (
        marc_file == NULL
    ) {

        search_selection = 0;

        return;
    }


    while (
        fgets(
            line,
            sizeof(line),
            marc_file
        ) != NULL
    ) {

        line[
            strcspn(
                line,
                "\r\n"
            )
        ] = '\0';


        if (
            strncmp(
                line,
                "GAME_ID=",
                8
            ) == 0
        ) {

            if (
                in_record &&
                record_matches
            ) {

                int game_index =
                    find_game_by_id(
                        current_game_id
                    );


                if (
                    game_index >= 0 &&
                    search_result_count <
                    MAX_SEARCH_RESULTS
                ) {

                    search_results[
                        search_result_count
                    ] = game_index;

                    search_result_count++;
                }
            }


            strncpy(
                current_game_id,
                line + 8,
                sizeof(current_game_id) - 1
            );

            current_game_id[
                sizeof(current_game_id) - 1
            ] = '\0';


            in_record = 1;


            record_matches =
                (
                    search_length == 0 ||
                    search_query[0] == '\0'
                );


            if (
                !record_matches &&
                contains_ignore_case(
                    current_game_id,
                    search_query
                )
            ) {

                record_matches = 1;
            }


            continue;
        }


        if (
            !in_record
        )
            continue;


        if (
            !record_matches &&
            contains_ignore_case(
                line,
                search_query
            )
        ) {

            record_matches = 1;
        }
    }


    if (
        in_record &&
        record_matches
    ) {

        int game_index =
            find_game_by_id(
                current_game_id
            );


        if (
            game_index >= 0 &&
            search_result_count <
            MAX_SEARCH_RESULTS
        ) {

            search_results[
                search_result_count
            ] = game_index;

            search_result_count++;
        }
    }


    fclose(
        marc_file
    );


    search_selection = 0;
}


void perform_search() {

    search_result_count = 0;


    if (
        search_mode ==
        SEARCH_MODE_MARC21
    ) {

        perform_marc_search();

        return;
    }


    for (
        int i = 0;
        i < game_count;
        i++
    ) {

        int match =
            contains_ignore_case(
                games[i].title,
                search_query
            );


        if (
            match &&
            search_result_count <
            MAX_SEARCH_RESULTS
        ) {

            search_results[
                search_result_count
            ] = i;

            search_result_count++;
        }
    }


    search_selection = 0;
}


void show_marc_menu() {

    printf(
        "\x1b[2J\x1b[H"
    );

    print_ui_line('=');

    print_centered(
        "MARC Records"
    );

    print_ui_line('=');

    printf("\n");


    if (
        game_count == 0
    ) {

        print_centered(
            "No games found."
        );

        printf("\n");

        print_centered(
            "B = Back"
        );

        return;
    }


    for (
        int i = 0;
        i < game_count;
        i++
    ) {

        print_menu_item(
            games[i].title,
            i == marc_selection
        );
    }


    printf("\n");


    {
        char line[80];

        snprintf(
            line,
            sizeof(line),
            "%d game(s) available.",
            game_count
        );

        print_centered(
            line
        );
    }


    printf("\n");

    print_centered(
        "UP / DOWN = Move"
    );

    print_centered(
        "A = View MARC Record"
    );

    print_centered(
        "B = Back"
    );
}


void show_marc_record() {

    int result =
        load_marc_record();


    printf(
        "\x1b[2J\x1b[H"
    );

    print_ui_line('=');

    print_centered(
        "MARC Record"
    );

    print_ui_line('=');

    printf("\n");


    if (
        result == -1
    ) {

        print_centered(
            "MARC database not found"
        );

        print_centered(
            "or could not be read."
        );

        printf("\n");

        print_centered(
            "Expected:"
        );

        print_centered(
            "sd:/marcviiew_marc.txt"
        );

        printf("\n");

        print_centered(
            "B = Back"
        );

        return;
    }


    if (
        result == 0
    ) {

        print_centered(
            "MARC record not found."
        );

        printf("\n");

        {
            char line[120];

            snprintf(
                line,
                sizeof(line),
                "Game: %s",
                games[
                    marc_selection
                ].title
            );

            print_centered(
                line
            );


            snprintf(
                line,
                sizeof(line),
                "ID: %s",
                games[
                    marc_selection
                ].id
            );

            print_centered(
                line
            );
        }

        printf("\n");

        print_centered(
            "B = Back"
        );

        return;
    }


    {
        char line[160];

        snprintf(
            line,
            sizeof(line),
            "Game: %s",
            games[
                marc_selection
            ].title
        );

        print_centered(
            line
        );


        snprintf(
            line,
            sizeof(line),
            "ID: %s",
            games[
                marc_selection
            ].id
        );

        print_centered(
            line
        );
    }


    printf("\n");


    int visible_lines = 15;

    int max_scroll =
        marc_line_count -
        visible_lines;

    if (
        max_scroll < 0
    )
        max_scroll = 0;


    if (
        marc_scroll >
        max_scroll
    )
        marc_scroll =
            max_scroll;


    if (
        marc_scroll < 0
    )
        marc_scroll = 0;


    for (
        int i = marc_scroll;
        i <
            marc_scroll +
            visible_lines &&
        i < marc_line_count;
        i++
    ) {

        print_centered(
            marc_lines[i]
        );
    }


    printf("\n");


    if (
        max_scroll > 0
    ) {

        char scroll_line[80];

        snprintf(
            scroll_line,
            sizeof(scroll_line),
            "UP / DOWN = Scroll  (%d/%d)",
            marc_scroll + 1,
            max_scroll + 1
        );

        print_centered(
            scroll_line
        );

    } else {

        print_centered(
            "UP / DOWN = Scroll"
        );
    }


    print_centered(
        "B = Back"
    );
}


/*
    Encode the entire MarcViiew MARC text database.

    This is an EXPORT operation.

    The normal LMS continues using marcviiew_marc.txt.
*/

void show_imported_menu(void){
    int i; printf("\\x1b[2J\\x1b[H"); print_ui_line('='); print_centered("Imported Records"); print_ui_line('='); printf("\\n");
    if(imported_record_count==0){print_centered("No imported records found.");printf("\\n");print_centered("Place .mrc files in:");print_centered("sd:/marcviiew_import");printf("\\n");print_centered("B = Back");return;}
    for(i=0;i<imported_record_count;i++) print_menu_item(imported_records[i].title,i==imported_selection);
    printf("\\n"); {char line[80];snprintf(line,sizeof(line),"%d imported record(s).",imported_record_count);print_centered(line);} printf("\\n");print_centered("UP / DOWN = Move");print_centered("A = View Record");print_centered("B = Back");
}
void show_imported_record(void){
    char path[256],line[200]; int installed; FILE *f; MARC_Record *r; MARC_Field *field;
    if(imported_record_count==0||imported_selection<0||imported_selection>=imported_record_count)return;
    snprintf(path,sizeof(path),"sd:/marcviiew/imported/%s",imported_records[imported_selection].filename); f=fopen(path,"rb");
    printf("\\x1b[2J\\x1b[H");print_ui_line('=');print_centered("Imported MARC Record");print_ui_line('=');printf("\\n");
    print_label_value("Title: ",imported_records[imported_selection].title);print_label_value("Game ID: ",imported_records[imported_selection].game_id);print_label_value("MARC 001: ",imported_records[imported_selection].marc_001);
    installed=find_game_by_id(imported_records[imported_selection].game_id); print_centered(installed>=0?"Status: Imported record / Game installed":"Status: Imported record / Game not installed");printf("\\n");
    if(!f){print_centered("Imported .mrc file could not be opened.");printf("\\n");print_centered("B = Back");return;}
    r=marc_record_create(); if(!r){fclose(f);print_centered("Could not create MARC record.");printf("\\n");print_centered("B = Back");return;}
    if(marc_record_read(r,f)!=0){fclose(f);marc_record_free(r);print_centered("Could not read imported MARC record.");printf("\\n");print_centered("B = Back");return;} fclose(f);
    marc_line_count=0;
    field=marc_record_get_field_by_tag(r,"245"); if(field&&imported_subfield(field,'a',line,sizeof(line))){char x[220];snprintf(x,sizeof(x),"245$a: %s",line);add_marc_line(x);}
    field=marc_record_get_field_by_tag(r,"264"); if(field&&imported_subfield(field,'a',line,sizeof(line))){char x[220];snprintf(x,sizeof(x),"264$a: %s",line);add_marc_line(x);} if(field&&imported_subfield(field,'c',line,sizeof(line))){char x[220];snprintf(x,sizeof(x),"264$c: %s",line);add_marc_line(x);}
    field=marc_record_get_field_by_tag(r,"300"); if(field&&imported_subfield(field,'a',line,sizeof(line))){char x[220];snprintf(x,sizeof(x),"300$a: %s",line);add_marc_line(x);}
    field=marc_record_get_field_by_tag(r,"542"); if(field&&imported_subfield(field,'i',line,sizeof(line))){char x[220];snprintf(x,sizeof(x),"542$i: %s",line);add_marc_line(x);} if(field&&imported_subfield(field,'k',line,sizeof(line))){char x[220];snprintf(x,sizeof(x),"542$k: %s",line);add_marc_line(x);} if(field&&imported_subfield(field,'s',line,sizeof(line))){char x[220];snprintf(x,sizeof(x),"542$s: %s",line);add_marc_line(x);}
    field=marc_record_get_field_by_tag(r,"655"); if(field){size_t i,n=marc_field_get_subfield_count(field);for(i=0;i<n;i++){MARC_Subfield *s=marc_field_get_subfield(field,i);if(s&&marc_subfield_get_code(s)=='a'){char x[220];snprintf(x,sizeof(x),"655$a: %s",marc_subfield_get_value(s));add_marc_line(x);}}}
    field=marc_record_get_field_by_tag(r,"520"); if(field&&imported_subfield(field,'a',line,sizeof(line))){add_marc_line("520$a:");add_marc_line(line);} marc_record_free(r);
    {int visible=10,max=marc_line_count-visible;if(max<0)max=0;if(imported_scroll>max)imported_scroll=max;if(imported_scroll<0)imported_scroll=0;for(int i=imported_scroll;i<imported_scroll+visible&&i<marc_line_count;i++)print_centered(marc_lines[i]);}
    printf("\\n");print_centered("UP / DOWN = Scroll");print_centered("B = Back");print_centered("+ = Main Menu");
}

void encode_entire_database() {

    int result;


    strcpy(
        encode_status,
        "Encoding entire MARC database..."
    );

    show_encode_marc();

    fflush(
        stdout
    );


    result =
        marcviiew_encode_database(
            "sd:/marcviiew_marc.txt",
            "sd:/marcviiew.mrc"
        );


    if (
        result == 0
    ) {

        snprintf(
            encode_status,
            sizeof(encode_status),
            "Complete database exported to sd:/marcviiew.mrc"
        );

    } else {

        strcpy(
            encode_status,
            "Database encoding failed."
        );
    }


    show_encode_marc();
}


/*
    Encode one selected Wii game.

    The selected game's ID comes from games[].

    The encoder searches marcviiew_marc.txt for the
    matching 001 / GAME_ID record and writes only
    that MARC record to its own ISO 2709 file.
*/
void encode_selected_game() {

    int result;

    char output_path[64];

    if (
        game_count == 0
    ) {

        strcpy(
            encode_status,
            "No games are available."
        );

        show_encode_marc();

        return;
    }


    if (
        encode_game_selection < 0 ||
        encode_game_selection >= game_count
    )
        return;


    snprintf(
        output_path,
        sizeof(output_path),
        "sd:/marcviiew_%s.mrc",
        games[
            encode_game_selection
        ].id
    );


    snprintf(
        encode_status,
        sizeof(encode_status),
        "Encoding %s...",
        games[
            encode_game_selection
        ].title
    );


    show_encode_marc();

    fflush(
        stdout
    );


    result =
        marcviiew_encode_game(
            "sd:/marcviiew_marc.txt",
            output_path,
            games[
                encode_game_selection
            ].id
        );


    if (
        result == 0
    ) {

        snprintf(
            encode_status,
            sizeof(encode_status),
            "Exported %s to %s",
            games[
                encode_game_selection
            ].id,
            output_path
        );

    } else {

        snprintf(
            encode_status,
            sizeof(encode_status),
            "Could not export %s.",
            games[
                encode_game_selection
            ].title
        );
    }


    show_encode_marc();
}


/*
    Encode MARC menu.

    0 = Encode Entire Database
    1 = Encode Selected Game
    2 = Back
*/
void show_encode_marc() {

    printf(
        "\x1b[2J\x1b[H"
    );

    print_ui_line('=');

    print_centered(
        "Encode MARC"
    );

    print_ui_line('=');

    printf("\n");


    print_menu_item(
        "Encode Entire Database",
        encode_selection == 0
    );

    print_menu_item(
        "Encode Selected Game",
        encode_selection == 1
    );

    print_menu_item(
        "Back",
        encode_selection == 2
    );


    printf("\n");


    if (
        strlen(
            encode_status
        ) > 0
    ) {

        print_centered(
            encode_status
        );

        printf("\n");
    }


    print_centered(
        "Export only - LMS uses marcviiew_marc.txt"
    );

    printf("\n");

    print_centered(
        "UP / DOWN = Move"
    );

    print_centered(
        "A = Select"
    );

    print_centered(
        "B = Back"
    );

    print_centered(
        "+ = Main Menu"
    );
}


/*
    Game picker for individual MARC export.
*/
void show_encode_game_menu() {

    printf(
        "\x1b[2J\x1b[H"
    );

    print_ui_line('=');

    print_centered(
        "Encode Selected Game"
    );

    print_ui_line('=');

    printf("\n");


    if (
        game_count == 0
    ) {

        print_centered(
            "No games found."
        );

        printf("\n");

        print_centered(
            "B = Back"
        );

        return;
    }


    /*
        Keep the game picker readable on the Wii.
        The selection can still move through all games.
    */
    int first =
        encode_game_selection - 5;

    if (
        first < 0
    )
        first = 0;


    int last =
        first + 10;

    if (
        last > game_count
    )
        last = game_count;


    if (
        last - first < 11
    ) {

        first =
            last - 11;

        if (
            first < 0
        )
            first = 0;
    }


    for (
        int i = first;
        i < last;
        i++
    ) {

        char line[160];

        snprintf(
            line,
            sizeof(line),
            "%s [%s]",
            games[i].title,
            games[i].id
        );

        print_menu_item(
            line,
            i == encode_game_selection
        );
    }


    printf("\n");


    {
        char line[100];

        snprintf(
            line,
            sizeof(line),
            "Game %d of %d",
            encode_game_selection + 1,
            game_count
        );

        print_centered(
            line
        );
    }


    printf("\n");

    print_centered(
        "UP / DOWN = Select Game"
    );

    print_centered(
        "A = Export MARC"
    );

    print_centered(
        "B = Back"
    );

    print_centered(
        "+ = Main Menu"
    );
}


void show_main_menu() {

    printf(
        "\x1b[2J\x1b[H"
    );

    printf("\n");

    print_ui_line('=');

    print_centered(
        "MarcViiew"
    );

    print_centered(
        "Wii Library Management System"
    );

    print_ui_line('=');

    printf("\n");


    print_menu_item(
        "Catalogue",
        menu_selection == 0
    );

    print_menu_item(
        "Search",
        menu_selection == 1
    );

    print_menu_item(
        "MARC Records",
        menu_selection == 2
    );

    print_menu_item(
        "Encode MARC",
        menu_selection == 3
    );

    print_menu_item(
        "Imported Records",
        menu_selection == 4
    );

    print_menu_item(
        "Settings",
        menu_selection == 5
    );


    printf("\n");

    print_centered(
        "UP / DOWN = Move    A = Select"
    );

    print_centered(
        "HOME = Exit"
    );
}


void show_catalogue() {

    printf(
        "\x1b[2J\x1b[H"
    );

    print_ui_line('=');

    print_centered(
        "Catalogue"
    );

    print_ui_line('=');

    printf("\n");


    if (
        game_count == 0
    ) {

        print_centered(
            "No games found."
        );

        printf("\n");

        print_centered(
            "B = Back"
        );

        return;
    }


    for (
        int i = 0;
        i < game_count;
        i++
    ) {

        print_menu_item(
            games[i].title,
            i == catalogue_selection
        );
    }


    printf("\n");


    {
        char line[80];

        snprintf(
            line,
            sizeof(line),
            "%d game(s) found.",
            game_count
        );

        print_centered(
            line
        );
    }


    printf("\n");

    print_centered(
        "UP / DOWN = Move"
    );

    print_centered(
        "A = View Game"
    );

    print_centered(
        "B = Back"
    );
}


void show_search_results() {

    printf(
        "\x1b[2J\x1b[H"
    );

    print_ui_line('=');


    if (
        search_mode ==
        SEARCH_MODE_MARC21
    )
        print_centered(
            "MARC21 Search Results"
        );
    else
        print_centered(
            "Search Results"
        );


    print_ui_line('=');

    printf("\n");


    {
        char line[160];

        snprintf(
            line,
            sizeof(line),
            "Search: %s",
            search_query
        );

        print_centered(
            line
        );
    }


    printf("\n");


    if (
        search_result_count == 0
    ) {

        print_centered(
            "No games found."
        );

        printf("\n");

        print_centered(
            "B = Back"
        );

        print_centered(
            "+ = Main Menu"
        );

        return;
    }


    for (
        int i = 0;
        i < search_result_count;
        i++
    ) {

        int game_index =
            search_results[i];


        print_menu_item(
            games[game_index].title,
            i == search_selection
        );
    }


    printf("\n");


    {
        char line[80];

        snprintf(
            line,
            sizeof(line),
            "%d result(s) found.",
            search_result_count
        );

        print_centered(
            line
        );
    }


    printf("\n");

    print_centered(
        "UP / DOWN = Move"
    );


    if (
        search_mode ==
        SEARCH_MODE_MARC21
    )
        print_centered(
            "A = View MARC Record"
        );
    else
        print_centered(
            "A = View Game"
        );


    print_centered(
        "B = Back"
    );

    print_centered(
        "+ = Main Menu"
    );
}


void show_settings_menu() {

    printf(
        "\x1b[2J\x1b[H"
    );

    print_ui_line('=');

    print_centered(
        "Settings"
    );

    print_ui_line('=');

    printf("\n");


    print_menu_item(
        "Reload SD / USB",
        settings_selection == 0
    );

    print_menu_item(
        "Reload Databases",
        settings_selection == 1
    );

    print_menu_item(
        "Credits",
        settings_selection == 2
    );


    printf("\n");


    if (
        strlen(settings_status) > 0
    ) {

        print_centered(
            settings_status
        );

        printf("\n");
    }


    print_centered(
        "UP / DOWN = Move"
    );

    print_centered(
        "A = Select"
    );

    print_centered(
        "B = Back"
    );

    print_centered(
        "+ = Main Menu"
    );
}


void settings_reload_storage() {

    int result;


    strcpy(
        settings_status,
        "Reloading SD / USB..."
    );

    show_settings_menu();

    fflush(
        stdout
    );


    usleep(
        200000
    );


    result =
        reload_storage();


    if (
        result
    ) {

        scan_catalogue();
        scan_imported_records();


        catalogue_selection = 0;
        marc_selection = 0;

        info_scroll = 0;
        marc_scroll = 0;

        search_selection = 0;
        search_result_count = 0;

        marc_record_from_search = 0;

        encode_selection = 0;
        encode_game_selection = 0;


        snprintf(
            settings_status,
            sizeof(settings_status),
            "SD / USB reloaded. %d game(s) found.",
            game_count
        );

    } else {

        strcpy(
            settings_status,
            "SD / USB reload failed."
        );
    }


    show_settings_menu();
}


void settings_reload_databases() {

    strcpy(
        settings_status,
        "Reloading databases..."
    );

    show_settings_menu();

    fflush(
        stdout
    );


    usleep(
        200000
    );


    reload_databases();


    show_settings_menu();
}


void show_credits() {

    printf(
        "\x1b[2J\x1b[H"
    );

    print_ui_line('=');

    print_centered(
        "Credits"
    );

    print_ui_line('=');

    printf("\n");


    print_centered(
        "MarcViiew"
    );

    print_centered(
        "Wii Library Management System"
    );


    printf("\n");

    print_centered(
        "Created by Amanda/Riruru. A library and Information Sciences student."
    );


    printf("\n");

    print_ui_line('-');

    printf("\n");


    print_centered(
        "Built with"
    );

    print_centered(
        "devkitPPC"
    );

    print_centered(
        "libogc"
    );

    print_centered(
        "ViiewLib"
    );

    printf("\n");

    print_ui_line('-');

    printf("\n");


    print_centered(
        "A Wii homebrew library"
    );

    print_centered(
        "management system."
    );


    printf("\n");

    print_centered(
        "B = Back"
    );

    print_centered(
        "+ = Main Menu"
    );
}


void show_search_mode_menu() {

    printf(
        "\x1b[2J\x1b[H"
    );

    printf("\n");

    print_ui_line('=');

    print_centered(
        "Search"
    );

    print_ui_line('=');

    printf("\n");

    print_centered(
        "Choose record type"
    );

    printf("\n");


    print_menu_item(
        "Normal Records",
        search_mode_selection == 0
    );

    print_menu_item(
        "MARC21 Records",
        search_mode_selection == 1
    );


    printf("\n");

    print_centered(
        "UP / DOWN = Move"
    );

    print_centered(
        "A = Select"
    );

    print_centered(
        "B = Back"
    );

    print_centered(
        "+ = Main Menu"
    );
}


void reset_keyboard() {

    keyboard_row = 0;

    keyboard_column = 0;

    keyboard_special_selection = 0;

    search_length = 0;

    search_query[0] = '\0';
}


void keyboard_add_character(
    char character
) {

    if (
        search_length <
        SEARCH_LENGTH - 1
    ) {

        search_query[
            search_length
        ] = character;

        search_length++;

        search_query[
            search_length
        ] = '\0';
    }
}


void keyboard_backspace() {

    if (
        search_length > 0
    ) {

        search_length--;

        search_query[
            search_length
        ] = '\0';
    }
}


void show_search_keyboard() {

    printf(
        "\x1b[2J\x1b[H"
    );

    printf("\n");

    print_ui_line('=');

    print_centered(
        "Search"
    );

    print_ui_line('=');

    printf("\n");


    {
        char query_line[160];

        snprintf(
            query_line,
            sizeof(query_line),
            "Query: %s_",
            search_query
        );

        print_centered(
            query_line
        );
    }


    printf("\n");


    if (
        search_mode ==
        SEARCH_MODE_MARC21
    )
        print_centered(
            "MARC21 Records"
        );
    else
        print_centered(
            "Normal Records"
        );


    printf("\n");


    for (
        int row = 0;
        row < KEYBOARD_ROWS;
        row++
    ) {

        char keyboard_line[64];

        int position = 0;


        for (
            int column = 0;
            column < KEYBOARD_COLUMNS;
            column++
        ) {

            if (
                row == 3 &&
                column >= 6
            ) {

                keyboard_line[
                    position++
                ] = ' ';

                keyboard_line[
                    position++
                ] = ' ';

                keyboard_line[
                    position++
                ] = ' ';

                keyboard_line[
                    position++
                ] = ' ';

                keyboard_line[
                    position++
                ] = ' ';

                continue;
            }


            keyboard_line[
                position++
            ] = '[';


            if (
                row == keyboard_row &&
                column == keyboard_column
            ) {

                keyboard_line[
                    position++
                ] = '>';

            } else {

                keyboard_line[
                    position++
                ] = ' ';
            }


            keyboard_line[
                position++
            ] = keyboard[
                row
            ][
                column
            ];

            keyboard_line[
                position++
            ] = ']';

            keyboard_line[
                position++
            ] = ' ';
        }


        keyboard_line[position] =
            '\0';


        print_centered(
            keyboard_line
        );
    }


    printf("\n");


    {
        char special_line[64];

        int position = 0;


        if (
            keyboard_row == 4 &&
            keyboard_special_selection == 0
        ) {

            const char *text =
                "[> SPACE ] ";

            strcpy(
                special_line + position,
                text
            );

            position += strlen(text);

        } else {

            const char *text =
                "[  SPACE ] ";

            strcpy(
                special_line + position,
                text
            );

            position += strlen(text);
        }


        if (
            keyboard_row == 4 &&
            keyboard_special_selection == 1
        ) {

            const char *text =
                "[>BACKSP] ";

            strcpy(
                special_line + position,
                text
            );

            position += strlen(text);

        } else {

            const char *text =
                "[ BACKSP ] ";

            strcpy(
                special_line + position,
                text
            );

            position += strlen(text);
        }


        if (
            keyboard_row == 4 &&
            keyboard_special_selection == 2
        ) {

            const char *text =
                "[>SEARCH ]";

            strcpy(
                special_line + position,
                text
            );

            position += strlen(text);

        } else {

            const char *text =
                "[ SEARCH ]";

            strcpy(
                special_line + position,
                text
            );

            position += strlen(text);
        }


        special_line[position] =
            '\0';


        print_centered(
            special_line
        );
    }


    printf("\n");

    print_centered(
        "D-PAD = Move"
    );

    print_centered(
        "A = Select"
    );

    print_centered(
        "B = Backspace"
    );

    print_centered(
        "+ = Main Menu"
    );
}


void keyboard_move_up() {

    if (
        keyboard_row == 4
    ) {

        keyboard_row = 3;

        if (
            keyboard_column > 5
        )
            keyboard_column = 5;

        return;
    }


    if (
        keyboard_row > 0
    ) {

        keyboard_row--;

        if (
            keyboard_row == 3 &&
            keyboard_column > 5
        )
            keyboard_column = 5;
    }
}


void keyboard_move_down() {

    if (
        keyboard_row < 4
    ) {

        keyboard_row++;

        if (
            keyboard_row == 4
        )
            keyboard_special_selection =
                keyboard_column > 2
                ? 2
                : keyboard_column;

        return;
    }
}


void keyboard_move_left() {

    if (
        keyboard_row == 4
    ) {

        if (
            keyboard_special_selection > 0
        )
            keyboard_special_selection--;

        return;
    }


    if (
        keyboard_column > 0
    )
        keyboard_column--;
}


void keyboard_move_right() {

    if (
        keyboard_row == 4
    ) {

        if (
            keyboard_special_selection < 2
        )
            keyboard_special_selection++;

        return;
    }


    int max_column = 9;

    if (
        keyboard_row == 3
    )
        max_column = 5;


    if (
        keyboard_column < max_column
    )
        keyboard_column++;
}


void keyboard_select() {

    if (
        keyboard_row < 4
    ) {

        char character =
            keyboard[
                keyboard_row
            ][
                keyboard_column
            ];


        keyboard_add_character(
            character
        );

        return;
    }


    if (
        keyboard_special_selection == 0
    ) {

        keyboard_add_character(
            ' '
        );

    }

    else if (
        keyboard_special_selection == 1
    ) {

        keyboard_backspace();

    }

    else if (
        keyboard_special_selection == 2
    ) {

        perform_search();

        screen = 6;

        show_search_results();
    }
}


void handle_search_keyboard(
    u32 input
) {

    if (
        input & INPUT_HOME
    ) {

        exit(0);
    }


    if (
        input & INPUT_PLUS
    ) {

        screen = 1;

        search_result_count = 0;

        marc_record_from_search = 0;

        show_main_menu();

        return;
    }


    if (
        input & INPUT_UP
    ) {

        keyboard_move_up();

        show_search_keyboard();

    }

    else if (
        input & INPUT_DOWN
    ) {

        keyboard_move_down();

        show_search_keyboard();

    }

    else if (
        input & INPUT_LEFT
    ) {

        keyboard_move_left();

        show_search_keyboard();

    }

    else if (
        input & INPUT_RIGHT
    ) {

        keyboard_move_right();

        show_search_keyboard();
    }


    if (
        input & INPUT_SELECT
    ) {

        keyboard_select();

        if (
            screen == 5
        )
            show_search_keyboard();
    }


    if (
        input & INPUT_BACK
    ) {

        keyboard_backspace();

        show_search_keyboard();
    }
}


void start_search() {

    search_mode_selection =
        search_mode;

    screen = 8;

    show_search_mode_menu();
}


void show_loading_screen(
    const char *message,
    int progress
) {

    const int bar_width = 30;

    int filled;
    int empty;


    if (
        progress < 0
    )
        progress = 0;

    if (
        progress > 100
    )
        progress = 100;


    filled =
        (progress * bar_width) / 100;

    empty =
        bar_width - filled;


    printf(
        "\x1b[2J\x1b[H"
    );


    printf("\n");


    print_centered(
        "=================================================="
    );

    printf("\n");


    print_centered(
        "MARCViiew"
    );

    print_centered(
        "Wii Library System"
    );

    printf("\n");


    print_centered(
        "--------------------------------------------------"
    );

    printf("\n");


    {
        char bar_line[40];

        int position = 0;


        bar_line[position++] = '+';


        for (
            int i = 0;
            i < filled;
            i++
        )
            bar_line[position++] = '#';


        for (
            int i = 0;
            i < empty;
            i++
        )
            bar_line[position++] = '-';


        bar_line[position++] = '+';

        bar_line[position] = '\0';


        print_centered(
            bar_line
        );
    }


    printf("\n");


    print_centered(
        message
    );

    printf("\n");


    {
        char percentage[20];

        snprintf(
            percentage,
            sizeof(percentage),
            "%3d%%",
            progress
        );

        print_centered(
            percentage
        );
    }


    printf("\n");


    print_centered(
        "--------------------------------------------------"
    );


    if (
        game_count > 0
    ) {

        printf("\n");

        {
            char game_count_line[50];

            snprintf(
                game_count_line,
                sizeof(game_count_line),
                "Games found: %d",
                game_count
            );

            print_centered(
                game_count_line
            );
        }
    }


    printf("\n");


    print_centered(
        "=================================================="
    );


    fflush(
        stdout
    );
}


/*
    Application entry point.
*/
int main(void)
{
    VIDEO_Init();

    WPAD_Init();

    fatInitDefault();


    rmode =
        VIDEO_GetPreferredMode(
            NULL
        );


    xfb =
        MEM_K0_TO_K1(
            SYS_AllocateFramebuffer(
                rmode
            )
        );


    console_init(
        xfb,
        20,
        20,
        rmode->fbWidth,
        rmode->xfbHeight,
        rmode->fbWidth *
        VI_DISPLAY_PIX_SZ
    );


    VIDEO_Configure(
        rmode
    );


    VIDEO_SetNextFramebuffer(
        xfb
    );

    VIDEO_SetBlack(
        false
    );

    VIDEO_Flush();

    VIDEO_WaitVSync();


    if (
        rmode->viTVMode &
        VI_NON_INTERLACE
    )
        VIDEO_WaitVSync();


    printf(
        "\x1b[2J\x1b[H"
    );

    printf("\n");


    print_centered(
        "=================================================="
    );

    printf("\n");


    print_centered(
        "MARCViiew"
    );

    print_centered(
        "Wii Library System"
    );

    printf("\n");


    print_centered(
        "--------------------------------------------------"
    );

    printf("\n");


    print_centered(
        "Press A to continue"
    );

    printf("\n");


    print_centered(
        "--------------------------------------------------"
    );

    printf("\n");


    print_centered(
        "HOME = Exit"
    );

    printf("\n");


    print_centered(
        "=================================================="
    );


    while (
        SYS_MainLoop()
    ) {

        WPAD_ScanPads();

        u32 splash_input =
            get_input();


        if (
            splash_input & INPUT_SELECT
        )
            break;


        if (
            splash_input & INPUT_HOME
        )
            exit(0);


        VIDEO_WaitVSync();
    }


    show_loading_screen(
        "Initialising system...",
        10
    );

    usleep(
        300000
    );


    show_loading_screen(
        "Scanning Wii games...",
        40
    );

    scan_catalogue();
    scan_imported_records();


    show_loading_screen(
        "Loading game metadata...",
        70
    );

    usleep(
        200000
    );


    show_loading_screen(
        "Preparing catalogue...",
        90
    );

    usleep(
        200000
    );


    show_loading_screen(
        "Loading complete!",
        100
    );

    usleep(
        500000
    );


    screen = 1;

    show_main_menu();


    while (
        SYS_MainLoop()
    ) {

        WPAD_ScanPads();

        u32 input =
            get_input();


        if (
            input & INPUT_HOME
        )
            exit(0);


        /*
            Search keyboard screen.
        */
        if (
            screen == 5
        ) {

            handle_search_keyboard(
                input
            );

            VIDEO_WaitVSync();

            continue;
        }


        /*
            Search mode selection screen.
        */
        if (
            screen == 8
        ) {

            if (
                input & INPUT_UP
            ) {

                if (
                    search_mode_selection > 0
                ) {

                    search_mode_selection--;

                    show_search_mode_menu();
                }
            }


            if (
                input & INPUT_DOWN
            ) {

                if (
                    search_mode_selection < 1
                ) {

                    search_mode_selection++;

                    show_search_mode_menu();
                }
            }


            if (
                input & INPUT_SELECT
            ) {

                search_mode =
                    search_mode_selection;

                marc_record_from_search = 0;

                reset_keyboard();

                screen = 5;

                show_search_keyboard();
            }


            if (
                input & INPUT_BACK
            ) {

                screen = 1;

                show_main_menu();
            }


            if (
                input & INPUT_PLUS
            ) {

                screen = 1;

                show_main_menu();
            }


            VIDEO_WaitVSync();

            continue;
        }


        if (screen == 13) {
            if(input & INPUT_UP){if(imported_selection>0){imported_selection--;show_imported_menu();}}
            if(input & INPUT_DOWN){if(imported_selection<imported_record_count-1){imported_selection++;show_imported_menu();}}
            if(input & INPUT_SELECT){if(imported_record_count>0){imported_scroll=0;screen=14;show_imported_record();}}
            if(input & INPUT_BACK){screen=1;show_main_menu();}
            if(input & INPUT_PLUS){screen=1;show_main_menu();}
            VIDEO_WaitVSync(); continue;
        }
        if (screen == 14) {
            if(input & INPUT_UP){imported_scroll--;show_imported_record();}
            if(input & INPUT_DOWN){imported_scroll++;show_imported_record();}
            if(input & INPUT_BACK){screen=13;show_imported_menu();}
            if(input & INPUT_PLUS){screen=1;show_main_menu();}
            VIDEO_WaitVSync(); continue;
        }

        /*
            Settings screen.
        */
        if (
            screen == 9
        ) {

            if (
                input & INPUT_UP
            ) {

                if (
                    settings_selection > 0
                ) {

                    settings_selection--;

                    show_settings_menu();
                }
            }


            if (
                input & INPUT_DOWN
            ) {

                if (
                    settings_selection < 2
                ) {

                    settings_selection++;

                    show_settings_menu();
                }
            }


            if (
                input & INPUT_SELECT
            ) {

                if (
                    settings_selection == 0
                ) {

                    settings_reload_storage();

                }

                else if (
                    settings_selection == 1
                ) {

                    settings_reload_databases();

                }

                else if (
                    settings_selection == 2
                ) {

                    screen = 10;

                    show_credits();
                }
            }


            if (
                input & INPUT_BACK
            ) {

                screen = 1;

                show_main_menu();
            }


            if (
                input & INPUT_PLUS
            ) {

                screen = 1;

                show_main_menu();
            }


            VIDEO_WaitVSync();

            continue;
        }


        /*
            Credits screen.
        */
        if (
            screen == 10
        ) {

            if (
                input & INPUT_BACK
            ) {

                screen = 9;

                show_settings_menu();
            }


            if (
                input & INPUT_PLUS
            ) {

                screen = 1;

                show_main_menu();
            }


            VIDEO_WaitVSync();

            continue;
        }


        /*
            Encode MARC menu.
        */
        if (
            screen == 11
        ) {

            if (
                input & INPUT_UP
            ) {

                if (
                    encode_selection > 0
                ) {

                    encode_selection--;

                    show_encode_marc();
                }
            }


            if (
                input & INPUT_DOWN
            ) {

                if (
                    encode_selection < 2
                ) {

                    encode_selection++;

                    show_encode_marc();
                }
            }


            if (
                input & INPUT_SELECT
            ) {

                if (
                    encode_selection == 0
                ) {

                    encode_entire_database();

                }

                else if (
                    encode_selection == 1
                ) {

                    encode_game_selection = 0;

                    screen = 12;

                    show_encode_game_menu();

                }

                else if (
                    encode_selection == 2
                ) {

                    screen = 1;

                    show_main_menu();
                }
            }


            if (
                input & INPUT_BACK
            ) {

                screen = 1;

                show_main_menu();
            }


            if (
                input & INPUT_PLUS
            ) {

                screen = 1;

                show_main_menu();
            }


            VIDEO_WaitVSync();

            continue;
        }


        /*
            Select Game for MARC export.
        */
        if (
            screen == 12
        ) {

            if (
                input & INPUT_UP
            ) {

                if (
                    encode_game_selection > 0
                ) {

                    encode_game_selection--;

                    show_encode_game_menu();
                }
            }


            if (
                input & INPUT_DOWN
            ) {

                if (
                    encode_game_selection <
                    game_count - 1
                ) {

                    encode_game_selection++;

                    show_encode_game_menu();
                }
            }


            if (
                input & INPUT_SELECT
            ) {

                encode_selected_game();
            }


            if (
                input & INPUT_BACK
            ) {

                screen = 11;

                show_encode_marc();
            }


            if (
                input & INPUT_PLUS
            ) {

                screen = 1;

                show_main_menu();
            }


            VIDEO_WaitVSync();

            continue;
        }


        /*
            A button handling.
        */
        if (
            input & INPUT_SELECT
        ) {

            if (
                screen == 1
            ) {

                if (
                    menu_selection == 0
                ) {

                    screen = 2;

                    catalogue_selection = 0;

                    show_catalogue();

                }

                else if (
                    menu_selection == 1
                ) {

                    start_search();

                }

                else if (
                    menu_selection == 2
                ) {

                    screen = 4;

                    marc_selection = 0;

                    marc_record_from_search = 0;

                    show_marc_menu();

                }

                else if (
                    menu_selection == 3
                ) {

                    screen = 11;

                    encode_selection = 0;

                    encode_game_selection = 0;

                    encode_status[0] = '\0';

                    show_encode_marc();

                }

                else if (
                    menu_selection == 4
                ) {
                    screen = 13;
                    imported_selection = 0;
                    imported_scroll = 0;
                    show_imported_menu();
                }

                else if (
                    menu_selection == 5
                ) {

                    screen = 9;

                    settings_selection = 0;

                    settings_status[0] = '\0';

                    show_settings_menu();
                }
            }


            else if (
                screen == 2
            ) {

                if (
                    game_count > 0
                ) {

                    screen = 3;

                    info_scroll = 0;

                    show_game_information();
                }
            }


            else if (
                screen == 4
            ) {

                if (
                    game_count > 0
                ) {

                    screen = 7;

                    marc_scroll = 0;

                    marc_record_from_search = 0;

                    show_marc_record();
                }
            }


            else if (
                screen == 6
            ) {

                if (
                    search_result_count > 0
                ) {

                    int selected_game =
                        search_results[
                            search_selection
                        ];


                    if (
                        search_mode ==
                        SEARCH_MODE_MARC21
                    ) {

                        marc_selection =
                            selected_game;

                        marc_scroll = 0;

                        marc_record_from_search = 1;

                        screen = 7;

                        show_marc_record();

                    } else {

                        catalogue_selection =
                            selected_game;

                        info_scroll = 0;

                        screen = 3;

                        show_game_information();
                    }
                }
            }
        }


        /*
            B button handling.
        */
        if (
            input & INPUT_BACK
        ) {

            if (
                screen == 3
            ) {

                if (
                    search_result_count > 0
                ) {

                    screen = 6;

                    show_search_results();

                } else {

                    screen = 2;

                    show_catalogue();
                }

            }

            else if (
                screen == 4
            ) {

                screen = 1;

                show_main_menu();

            }

            else if (
                screen == 7
            ) {

                if (
                    marc_record_from_search
                ) {

                    screen = 6;

                    show_search_results();

                } else {

                    screen = 4;

                    show_marc_menu();
                }

            }

            else if (
                screen == 2
            ) {

                screen = 1;

                show_main_menu();

            }

            else if (
                screen == 6
            ) {

                screen = 5;

                show_search_keyboard();
            }

            else if (screen == 13) {
                screen = 1;
                show_main_menu();
            }

            else if (screen == 14) {
                screen = 13;
                show_imported_menu();
            }
        }


        /*
            PLUS = Main Menu.
        */
        if (
            input & INPUT_PLUS
        ) {

            if (
                screen != 0 &&
                screen != 1
            ) {

                screen = 1;

                search_result_count = 0;

                marc_record_from_search = 0;

                show_main_menu();
            }
        }


        /*
            DOWN navigation.
        */
        if (
            input & INPUT_DOWN
        ) {

            if (
                screen == 1
            ) {

                if (
                    menu_selection < 5
                ) {

                    menu_selection++;

                    show_main_menu();
                }

            }


            else if (
                screen == 2
            ) {

                if (
                    catalogue_selection <
                    game_count - 1
                ) {

                    catalogue_selection++;

                    show_catalogue();
                }

            }


            else if (
                screen == 3
            ) {

                info_scroll++;

                show_game_information();

            }


            else if (
                screen == 4
            ) {

                if (
                    marc_selection <
                    game_count - 1
                ) {

                    marc_selection++;

                    show_marc_menu();
                }

            }


            else if (
                screen == 7
            ) {

                marc_scroll++;

                show_marc_record();

            }


            else if (
                screen == 6
            ) {

                if (
                    search_selection <
                    search_result_count - 1
                ) {

                    search_selection++;

                    show_search_results();
                }
            }
        }


        /*
            UP navigation.
        */
        if (
            input & INPUT_UP
        ) {

            if (
                screen == 1
            ) {

                if (
                    menu_selection > 0
                ) {

                    menu_selection--;

                    show_main_menu();
                }

            }


            else if (
                screen == 2
            ) {

                if (
                    catalogue_selection > 0
                ) {

                    catalogue_selection--;

                    show_catalogue();
                }

            }


            else if (
                screen == 3
            ) {

                info_scroll--;

                show_game_information();

            }


            else if (
                screen == 4
            ) {

                if (
                    marc_selection > 0
                ) {

                    marc_selection--;

                    show_marc_menu();
                }

            }


            else if (
                screen == 7
            ) {

                marc_scroll--;

                show_marc_record();

            }


            else if (
                screen == 6
            ) {

                if (
                    search_selection > 0
                ) {

                    search_selection--;

                    show_search_results();
                }
            }
        }


        VIDEO_WaitVSync();
    }


    return 0;
}