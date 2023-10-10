

void iniciarAParada() {
	printf("        --*:-*#*:::***$-:.++            db   dD db    db  .d8b.  .d8888. d88888b\n");
	printf("       @@Y ..--Y*.**#*--.    @@--       88 ,8P' 88    88 d8' `8b 88'  YP 88'\n");
	printf("     MMY ........    ...       mm--     88,8P   88    88 88ooo88 `8bo.   88ooooo\n");
	printf("    $##------.....y....          MM     88`8b   88    88 88~~~88   `Y8b. 88~~~~~.\n");
	printf("   @@--::::--------....                 88 `88. 88b  d88 88   88 db   8D 88.\n");
	printf("   --::::::::----........          ++   YP   YD ~Y8888P' YP   YP `8888Y' Y88888P\n");
	printf("   --::::::----.............       --   \n");
	printf("   --::::::::----.........         --             888      d8b\n");
	printf("   ----::+#@@mm----..--mmMM@@++    ++             888      Y8P\n");
	printf("   ::::++*#####@@++..--++mm::....  --             888 \n");
	printf("   :::-+mmm##Mmm:..  --####MM....  --             888      888 88888b.  888  888 888  888\n");
	printf("   +::.-+#@@MMm      ..@@MM------                 888      888 88888b.  888  888 888  888 \n");
	printf("   :::..:.-++--::         ------    ::            888      888 888 '88b 888  888 `Y8bd8P'\n");
	printf("   ::::::::::++mm--..  ......      ++             888      888 888  888 888  888   X88K\n");
	printf("   MM::----::MM::--..  --::..      MM             888      888 888  888 Y88b 888 .d8''8b.\n");
	printf("   @@  --::::MM@@--..--::MM--      mm             88888888 888 888  888  'Y88888 888  888\n");
	printf("   :M--++::##################--   -#:    \n");
	printf("   :. :::MM####Y==+---+==###MM+:... $        Pressione Qualquer Tecla para continuar ...\n");
	printf("     Y::.mm@#--__.....__-++##::   #m     \n");
	printf("     $:::::----mmMM####@@..  ++--  --    \n");
	printf("     ###:...--  mmmm####--  ..::--Y.:    \n");
	printf("     --##y*::::::Y###y--:::----###$      \n");
	printf("       .@####:::::**::::...:YM####       \n");
	printf("         @###@@####@@##-..y##@@--        \n");
	printf("          --################--           \n");
	printf("               #######@@Y                \n");

}

enum cores 
{   
    DARKBLUE = 1, 
    DARKGREEN = 2,
    LIGHTBLUE = 3,
    RED = 4,
    PURPLE = 5,
    YELLOW = 6,
    WHITE = 7,
    GRAY = 8,
    BLUE = 9,
    GREEN = 10
};

void textcolor (int color)
{
    static int __BACKGROUND;

    HANDLE h = GetStdHandle ( STD_OUTPUT_HANDLE );
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;


    GetConsoleScreenBufferInfo(h, &csbiInfo);

    SetConsoleTextAttribute (GetStdHandle (STD_OUTPUT_HANDLE),
                            color + (__BACKGROUND << 4));
}