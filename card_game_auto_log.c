#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_PLAYERS 4
#define HAND_SIZE 8
#define MAX_CARD 52
#define LOG_FILE_NAME "game_log.txt"

typedef enum {
    HEART = 0,
    SPADE,
    CLUB,
    DIAMOND
} Suit;

typedef struct {
    int value;  // 1~13
    Suit suit;
} Card;

typedef struct {
    char name[16];
    Card hand[HAND_SIZE];
    int cardCount;
    int score;
} Player;

typedef struct {
    Player players[MAX_PLAYERS];
    int playerCount;
    Card deck[MAX_CARD];
    int autoMode;    // 1 = 全自动模式，0 = 手动弃牌模式
    int actionNo;    // 用于详细日志中的行动编号
} Game;

static FILE *g_logFile = NULL;
static const char *SUIT_NAME[] = {"♥", "♠", "♣", "♦"};

// 同时输出到控制台和日志文件
void logMsg(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    if (g_logFile != NULL) {
        va_start(args, fmt);
        vfprintf(g_logFile, fmt, args);
        va_end(args);
        fflush(g_logFile);
    }
    fflush(stdout);
}

void printCard(Card c) {
    logMsg("%s%d", SUIT_NAME[c.suit], c.value);
}

// 打印手牌，label 需要自带“：”之类的结尾
void showHand(Player *p, const char *label) {
    logMsg("%s", label);
    if (p->cardCount == 0) {
        logMsg("(空)\n");
        return;
    }
    for (int i = 0; i < p->cardCount; i++) {
        if (i > 0) logMsg(" ");
        printCard(p->hand[i]);
    }
    logMsg("\n");
}

void initDeck(Game *game) {
    int index = 0;
    for (int s = HEART; s <= DIAMOND; s++) {
        for (int v = 1; v <= 13; v++) {
            game->deck[index].value = v;
            game->deck[index].suit = (Suit)s;
            index++;
        }
    }
}

void shuffleDeck(Game *game) {
    for (int i = 0; i < MAX_CARD; i++) {
        int r = rand() % MAX_CARD;
        Card temp = game->deck[i];
        game->deck[i] = game->deck[r];
        game->deck[r] = temp;
    }
}

void dealCards(Game *game) {
    int index = 0;
    for (int i = 0; i < game->playerCount; i++) {
        game->players[i].cardCount = 0;
        game->players[i].score = 0;
        for (int j = 0; j < HAND_SIZE; j++) {
            game->players[i].hand[game->players[i].cardCount++] = game->deck[index++];
        }
    }
}

void removeFrontCards(Player *p, int n) {
    if (n <= 0 || n > p->cardCount) return;
    for (int i = 0; i + n < p->cardCount; i++) {
        p->hand[i] = p->hand[i + n];
    }
    p->cardCount -= n;
}

void discardOneCard(Player *p, int index) {
    if (index < 0 || index >= p->cardCount) return;
    for (int i = index; i < p->cardCount - 1; i++) {
        p->hand[i] = p->hand[i + 1];
    }
    p->cardCount--;
}

void sortPlayersByScore(Player players[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (players[i].score < players[j].score) {
                Player temp = players[i];
                players[i] = players[j];
                players[j] = temp;
            }
        }
    }
}

// 手动模式：请玩家输入；自动模式：默认弃掉最后一张（可在此扩展更聪明的策略）
int chooseDiscardIndex(Game *game, Player *p) {
    if (game->autoMode) {
        logMsg("  （自动策略：弃掉最后一张）\n");
        return p->cardCount - 1;
    }

    int input = -1;
    logMsg("  请输入要弃掉的手牌序号（0~%d，输入 -1 表示默认最后一张）：",
           p->cardCount - 1);
    scanf("%d", &input);

    if (input >= 0 && input < p->cardCount) {
        return input;
    }
    logMsg("  输入无效，按默认规则弃掉最后一张。\n");
    return p->cardCount - 1;
}

// ======================= 弃牌阶段 =======================

void discardPhase(Game *game) {
    logMsg("\n==================== 弃牌阶段 ====================\n");
    logMsg("每位玩家轮流弃掉 1 张牌。\n");

    for (int i = 0; i < game->playerCount; i++) {
        Player *p = &game->players[i];

        logMsg("\n[行动 %d] 轮到 %s 弃牌\n", ++game->actionNo, p->name);
        showHand(p, "  弃牌前手牌：");

        int index = chooseDiscardIndex(game, p);
        logMsg("  %s 弃掉第 %d 张：", p->name, index + 1);
        printCard(p->hand[index]);
        logMsg("\n");

        discardOneCard(p, index);
        showHand(p, "  弃牌后手牌：");
    }

    logMsg("\n弃牌阶段结束。\n");
}

// ======================= 四局回合逻辑 =======================

// 第一局：轮流出 1 张，点数最小的玩家扣 1 分
void round1(Game *game) {
    logMsg("\n==================== 第 1 局 ====================\n");
    logMsg("规则：每人轮流打出 1 张牌，点数最小者扣 1 分。\n");

    Card played[MAX_PLAYERS];
    int owner[MAX_PLAYERS];
    int playedCount = 0;
    int minValue = 999;

    for (int i = 0; i < game->playerCount; i++) {
        Player *p = &game->players[i];
        if (p->cardCount < 1) {
            logMsg("  %s 手牌不足，本局不参与。\n", p->name);
            continue;
        }

        logMsg("\n[行动 %d] 轮到 %s 出牌\n", ++game->actionNo, p->name);
        showHand(p, "  出牌前手牌：");

        Card c = p->hand[0];
        played[playedCount] = c;
        owner[playedCount] = i;
        playedCount++;
        if (c.value < minValue) minValue = c.value;

        logMsg("  %s 打出：", p->name);
        printCard(c);
        logMsg("\n");

        removeFrontCards(p, 1);
        showHand(p, "  出牌后手牌：");
    }

    logMsg("\n结算：本局共 %d 人出牌，最小点数 = %d\n", playedCount, minValue);
    for (int j = 0; j < playedCount; j++) {
        if (played[j].value == minValue) {
            Player *p = &game->players[owner[j]];
            p->score -= 1;
            logMsg("  %s 点数最小，扣 1 分，当前分数：%d\n", p->name, p->score);
        }
    }
    logMsg("第 1 局结束。\n");
}

// 第二局：轮流出 1 张，点数最大的玩家扣 1 分
void round2(Game *game) {
    logMsg("\n==================== 第 2 局 ====================\n");
    logMsg("规则：每人轮流打出 1 张牌，点数最大者扣 1 分。\n");

    Card played[MAX_PLAYERS];
    int owner[MAX_PLAYERS];
    int playedCount = 0;
    int maxValue = -1;

    for (int i = 0; i < game->playerCount; i++) {
        Player *p = &game->players[i];
        if (p->cardCount < 1) {
            logMsg("  %s 手牌不足，本局不参与。\n", p->name);
            continue;
        }

        logMsg("\n[行动 %d] 轮到 %s 出牌\n", ++game->actionNo, p->name);
        showHand(p, "  出牌前手牌：");

        Card c = p->hand[0];
        played[playedCount] = c;
        owner[playedCount] = i;
        playedCount++;
        if (c.value > maxValue) maxValue = c.value;

        logMsg("  %s 打出：", p->name);
        printCard(c);
        logMsg("\n");

        removeFrontCards(p, 1);
        showHand(p, "  出牌后手牌：");
    }

    logMsg("\n结算：本局共 %d 人出牌，最大点数 = %d\n", playedCount, maxValue);
    for (int j = 0; j < playedCount; j++) {
        if (played[j].value == maxValue) {
            Player *p = &game->players[owner[j]];
            p->score -= 1;
            logMsg("  %s 点数最大，扣 1 分，当前分数：%d\n", p->name, p->score);
        }
    }
    logMsg("第 2 局结束。\n");
}

// 第三局：21 点，每人轮流打出 2 张，总和最接近 21 且不超过 21 的玩家赢，其余人扣 2 分
void round3(Game *game) {
    logMsg("\n==================== 第 3 局 ====================\n");
    logMsg("规则：21 点。每人轮流打出 2 张牌，总点数最接近 21 且不超过 21 者赢，其他参与者扣 2 分。\n");

    int total[MAX_PLAYERS];
    int bestIndex = -1;
    int bestScore = -1;

    for (int i = 0; i < game->playerCount; i++) {
        Player *p = &game->players[i];
        total[i] = -1;

        if (p->cardCount < 2) {
            logMsg("  %s 手牌不足 2 张，本局不参与。\n", p->name);
            continue;
        }

        logMsg("\n[行动 %d] 轮到 %s 出牌\n", ++game->actionNo, p->name);
        showHand(p, "  出牌前手牌：");

        Card c1 = p->hand[0];
        Card c2 = p->hand[1];
        int sum = c1.value + c2.value;
        total[i] = sum;

        logMsg("  %s 打出两张：", p->name);
        printCard(c1);
        logMsg(" + ");
        printCard(c2);
        logMsg(" = %d\n", sum);

        removeFrontCards(p, 2);
        showHand(p, "  出牌后手牌：");

        if (sum <= 21 && (bestIndex == -1 || sum > bestScore)) {
            bestIndex = i;
            bestScore = sum;
        }
    }

    if (bestIndex == -1) {
        logMsg("\n结算：无人不超过 21 点，所有参与者扣 2 分。\n");
        for (int i = 0; i < game->playerCount; i++) {
            if (total[i] != -1) {
                Player *p = &game->players[i];
                p->score -= 2;
                logMsg("  %s 扣 2 分，当前分数：%d\n", p->name, p->score);
            }
        }
    } else {
        logMsg("\n结算：赢家为 %s，点数为 %d\n",
               game->players[bestIndex].name, bestScore);
        for (int i = 0; i < game->playerCount; i++) {
            if (i != bestIndex && total[i] != -1) {
                Player *p = &game->players[i];
                p->score -= 2;
                logMsg("  %s 输给赢家，扣 2 分，当前分数：%d\n", p->name, p->score);
            }
        }
    }
    logMsg("第 3 局结束。\n");
}

// 第四局：每人使用剩下的 3 张牌，总和最小的玩家扣 4 分
void round4(Game *game) {
    logMsg("\n==================== 第 4 局 ====================\n");
    logMsg("规则：每人轮流打出剩余 3 张牌，总点数最小者扣 4 分。\n");

    int sum[MAX_PLAYERS];
    int loserIndex = -1;
    int minSum = 999;

    for (int i = 0; i < game->playerCount; i++) {
        Player *p = &game->players[i];
        sum[i] = -1;

        if (p->cardCount < 3) {
            logMsg("  %s 手牌不足 3 张，本局不参与。\n", p->name);
            continue;
        }

        logMsg("\n[行动 %d] 轮到 %s 出牌\n", ++game->actionNo, p->name);
        showHand(p, "  出牌前手牌：");

        Card c1 = p->hand[0];
        Card c2 = p->hand[1];
        Card c3 = p->hand[2];
        int total = c1.value + c2.value + c3.value;
        sum[i] = total;

        logMsg("  %s 打出三张：", p->name);
        printCard(c1);
        logMsg(" + ");
        printCard(c2);
        logMsg(" + ");
        printCard(c3);
        logMsg(" = %d\n", total);

        removeFrontCards(p, 3);
        showHand(p, "  出牌后手牌：");

        if (total < minSum) {
            minSum = total;
            loserIndex = i;
        }
    }

    if (loserIndex != -1) {
        Player *loser = &game->players[loserIndex];
        logMsg("\n结算：%s 的三张牌总和最小（%d），扣 4 分。\n",
               loser->name, minSum);
        loser->score -= 4;
        logMsg("  %s 当前分数：%d\n", loser->name, loser->score);
    } else {
        logMsg("\n结算：本局无人参与。\n");
    }
    logMsg("第 4 局结束。\n");
}

// ======================= 主流程 =======================

int main() {
#ifdef _WIN32
    // 让 Windows 控制台按 UTF-8 显示中文字符和花色符号
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    g_logFile = fopen(LOG_FILE_NAME, "w");
    if (g_logFile == NULL) {
        printf("警告：无法创建日志文件 %s，本次只输出到控制台。\n",
               LOG_FILE_NAME);
    }

    Game game;
    game.actionNo = 0;

    logMsg("========================================\n");
    logMsg("欢迎来到纸牌小游戏（自动轮流出牌 + 详细日志版）！\n");
    logMsg("========================================\n");

    time_t now = time(NULL);
    struct tm *tmNow = localtime(&now);
    char timeBuf[64];
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", tmNow);
    logMsg("本局开始时间：%s\n", timeBuf);
    if (g_logFile != NULL) {
        logMsg("详细日志文件：%s\n", LOG_FILE_NAME);
    }

    int mode = 0;
    logMsg("\n请选择游戏模式：\n");
    logMsg("  1. 手动弃牌模式（出牌阶段仍按玩家顺序自动执行）\n");
    logMsg("  2. 全自动模式（弃牌和出牌全部自动执行）\n");
    logMsg("请输入模式编号（1 或 2）：");
    scanf("%d", &mode);
    game.autoMode = (mode == 2) ? 1 : 0;
    logMsg("当前模式：%s\n", game.autoMode ? "全自动模式" : "手动弃牌模式");

    logMsg("请输入玩家人数（2~4）：");
    scanf("%d", &game.playerCount);
    if (game.playerCount < 2 || game.playerCount > MAX_PLAYERS) {
        logMsg("玩家人数必须在 2 到 4 之间！\n");
        if (g_logFile != NULL) fclose(g_logFile);
        return 1;
    }

    for (int i = 0; i < game.playerCount; i++) {
        logMsg("请输入第 %d 个玩家名字：", i + 1);
        scanf("%s", game.players[i].name);
        game.players[i].score = 0;
        game.players[i].cardCount = 0;
    }

    unsigned seed = (unsigned)time(NULL);
    srand(seed);
    logMsg("\n随机种子：%u\n", seed);
    logMsg("玩家名单：");
    for (int i = 0; i < game.playerCount; i++) {
        if (i > 0) logMsg("、");
        logMsg("%s", game.players[i].name);
    }
    logMsg("\n");

    initDeck(&game);
    shuffleDeck(&game);
    dealCards(&game);

    logMsg("\n==================== 发牌结果 ====================\n");
    for (int i = 0; i < game.playerCount; i++) {
        logMsg("%s 的初始手牌：", game.players[i].name);
        showHand(&game.players[i], "");
    }

    discardPhase(&game);

    round1(&game);
    round2(&game);
    round3(&game);
    round4(&game);

    logMsg("\n==================== 最终分数 ====================\n");
    for (int i = 0; i < game.playerCount; i++) {
        logMsg("%s：%d\n", game.players[i].name, game.players[i].score);
    }

    sortPlayersByScore(game.players, game.playerCount);
    logMsg("\n==================== 排名 ====================\n");
    for (int i = 0; i < game.playerCount; i++) {
        logMsg("%d. %s  分数：%d\n", i + 1,
               game.players[i].name, game.players[i].score);
    }

    if (g_logFile != NULL) {
        logMsg("\n（本次游戏已写入 %s）\n", LOG_FILE_NAME);
        fclose(g_logFile);
        g_logFile = NULL;
    }

    return 0;
}
