//test
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_PLAYERS 4
#define HAND_SIZE 8
#define MAX_CARD 52

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
} Game;

// ======================= 基础函数 =======================

void printCard(Card c) {
    const char* suitName[] = {"♥", "♠", "♣", "♦"};
    printf("%s%d", suitName[c.suit], c.value);
}

void printHand(Player *p) {
    printf("%s 的手牌：", p->name);
    for (int i = 0; i < p->cardCount; i++) {
        printCard(p->hand[i]);
        printf("  ");
    }
    printf("\n");
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

// ======================= 回合逻辑 =======================

// 第一局：每人出一张牌，最小的扣一分
void round1(Game *game) {
    printf("\n===== 第 1 局：每人出 1 张牌，最小的扣 1 分 =====\n");

    int minValue = 999;
    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].cardCount < 1) continue;

        Card c = game->players[i].hand[0];
        printf("%s 出：", game->players[i].name);
        printCard(c);
        printf("\n");

        if (c.value < minValue) {
            minValue = c.value;
        }
    }

    printf("本轮最小牌值：%d\n", minValue);

    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].cardCount >= 1 && game->players[i].hand[0].value == minValue) {
            game->players[i].score -= 1;
            printf("%s 扣 1 分\n", game->players[i].name);
        }
    }

    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].cardCount >= 1) {
            removeFrontCards(&game->players[i], 1);
        }
    }
}

// 第二局：每人出一张牌，最大的扣一分
void round2(Game *game) {
    printf("\n===== 第 2 局：每人出 1 张牌，最大的扣 1 分 =====\n");

    int maxValue = -1;
    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].cardCount < 1) continue;

        Card c = game->players[i].hand[0];
        printf("%s 出：", game->players[i].name);
        printCard(c);
        printf("\n");

        if (c.value > maxValue) {
            maxValue = c.value;
        }
    }

    printf("本轮最大牌值：%d\n", maxValue);

    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].cardCount >= 1 && game->players[i].hand[0].value == maxValue) {
            game->players[i].score -= 1;
            printf("%s 扣 1 分\n", game->players[i].name);
        }
    }

    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].cardCount >= 1) {
            removeFrontCards(&game->players[i], 1);
        }
    }
}

// 第三局：21点，每人出两张牌，最接近21且不超过21的赢，其他人扣2分
void round3(Game *game) {
    printf("\n===== 第 3 局：21 点，每人出 2 张牌 =====\n");

    int bestIndex = -1;
    int bestScore = -1;

    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].cardCount < 2) continue;

        int total = game->players[i].hand[0].value + game->players[i].hand[1].value;

        printf("%s 的两张牌：", game->players[i].name);
        printCard(game->players[i].hand[0]);
        printf(" + ");
        printCard(game->players[i].hand[1]);
        printf(" = %d\n", total);

        if (total <= 21 && (bestIndex == -1 || total > bestScore)) {
            bestIndex = i;
            bestScore = total;
        }
    }

    if (bestIndex == -1) {
        printf("本轮无人不超过 21，所有玩家扣 2 分。\n");
        for (int i = 0; i < game->playerCount; i++) {
            if (game->players[i].cardCount >= 2) {
                game->players[i].score -= 2;
            }
        }
    } else {
        printf("本轮赢家：%s，点数为 %d\n", game->players[bestIndex].name, bestScore);
        for (int i = 0; i < game->playerCount; i++) {
            if (i != bestIndex && game->players[i].cardCount >= 2) {
                game->players[i].score -= 2;
                printf("%s 扣 2 分\n", game->players[i].name);
            }
        }
    }

    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].cardCount >= 2) {
            removeFrontCards(&game->players[i], 2);
        }
    }
}

// 第四局：每人使用剩下 3 张牌比牌，输家扣 4 分
void round4(Game *game) {
    printf("\n===== 第 4 局：每人使用剩下 3 张牌比牌 =====\n");

    int loserIndex = -1;
    int minSum = 999;

    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].cardCount < 3) continue;

        int total = game->players[i].hand[0].value +
                    game->players[i].hand[1].value +
                    game->players[i].hand[2].value;

        printf("%s 的三张牌和为：%d\n", game->players[i].name, total);

        if (total < minSum) {
            minSum = total;
            loserIndex = i;
        }
    }

    if (loserIndex != -1) {
        printf("本轮输家：%s，扣 4 分\n", game->players[loserIndex].name);
        game->players[loserIndex].score -= 4;
    }

    for (int i = 0; i < game->playerCount; i++) {
        if (game->players[i].cardCount >= 3) {
            removeFrontCards(&game->players[i], 3);
        }
    }
}

// ======================= 主流程 =======================

int main() {
    srand((unsigned)time(NULL));
    Game game;

    printf("欢迎来到纸牌小游戏！\n");
    printf("请输入玩家人数（2~4）：");
    scanf("%d", &game.playerCount);

    if (game.playerCount < 2 || game.playerCount > MAX_PLAYERS) {
        printf("玩家人数必须在 2 到 4 之间！\n");
        return 1;
    }

    for (int i = 0; i < game.playerCount; i++) {
        printf("请输入第 %d 个玩家名字：", i + 1);
        scanf("%s", game.players[i].name);
        game.players[i].score = 0;
    }

    initDeck(&game);
    shuffleDeck(&game);
    dealCards(&game);

    printf("\n发牌完成，开始弃牌阶段。\n");
    for (int i = 0; i < game.playerCount; i++) {
        printHand(&game.players[i]);

        int discardIndex = game.players[i].cardCount - 1; // 默认弃掉最后一张
        printf("%s 选择弃掉第 %d 张牌（输入 0~%d，默认最后一张）：",
               game.players[i].name,
               discardIndex + 1,
               game.players[i].cardCount - 1);

        int input = -1;
        scanf("%d", &input);

        if (input >= 0 && input < game.players[i].cardCount) {
            discardIndex = input;
        }

        discardOneCard(&game.players[i], discardIndex);
        printf("弃牌后：\n");
        printHand(&game.players[i]);
    }

    // 四局游戏
    round1(&game);
    round2(&game);
    round3(&game);
    round4(&game);

    printf("\n===== 最终分数 =====\n");
    for (int i = 0; i < game.playerCount; i++) {
        printf("%s：%d\n", game.players[i].name, game.players[i].score);
    }

    // 排名
    sortPlayersByScore(game.players, game.playerCount);
    printf("\n===== 排名 =====\n");
    for (int i = 0; i < game.playerCount; i++) {
        printf("%d. %s  分数：%d\n", i + 1, game.players[i].name, game.players[i].score);
    }

    return 0;
}