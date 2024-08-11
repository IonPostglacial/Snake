#include <stdint.h>
#include <stdbool.h>

extern void canvas_set_fill_style(uint32_t color);
extern void canvas_fill_rect(int32_t x, int32_t y, int32_t width, int32_t height);
extern void canvas_fill(void);
extern void snake_score_changed(int32_t score);
extern void snake_step_period_updated(int32_t period);
extern void snake_game_over(void);
extern int32_t js_random(int32_t max);

#define COLOR_BACKGROUND 0x00000000
#define COLOR_SNAKE 0x00ff00
#define COLOR_APPLE 0xff0000
#define CELL_SIZE 10
#define GRID_WIDTH 40
#define GRID_HEIGHT 40

typedef enum {
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
} Direction;

typedef enum {
    KEY_CODE_ARROW_UP,
    KEY_CODE_ARROW_DOWN,
    KEY_CODE_ARROW_LEFT,
    KEY_CODE_ARROW_RIGHT
} KeyCode;

bool direction_is_opposite(Direction dir, Direction other) {
    return dir == DIRECTION_UP && other == DIRECTION_DOWN ||
           dir == DIRECTION_DOWN && other == DIRECTION_UP ||
           dir == DIRECTION_LEFT && other == DIRECTION_RIGHT ||
           dir == DIRECTION_RIGHT && other == DIRECTION_LEFT;
}

typedef struct {
    int32_t x;
    int32_t y;
} Position;

bool position_equals(Position pos, Position other) {
    return pos.x == other.x && pos.y == other.y;
}

Position position_moved(Position pos, Direction direction) {
    switch (direction) {
    case DIRECTION_UP:
        pos.y--;
        break;
    case DIRECTION_DOWN:
        pos.y++;
        break;
    case DIRECTION_LEFT:
        pos.x--;
        break;
    case DIRECTION_RIGHT:
        pos.x++;
        break;
    }
    return pos;
}

typedef struct {
    Position segments[GRID_WIDTH * GRID_HEIGHT];
    int32_t length;
    int32_t head_index;
    Direction direction;
} Snake;

Position snake_head_position(Snake *snake) {
    return snake->segments[snake->head_index];
}

Position snake_next_head_position(Snake *snake) {
    return position_moved(snake->segments[snake->head_index], snake->direction);
}

bool snake_eats_himself(Snake *snake) {
    for (int32_t i = 0; i < snake->length; i++) {
        if (i == snake->head_index) {
            continue;
        }
        if (position_equals(snake_head_position(snake), snake->segments[i])) {
            return true;
        }
    }
    return false;
}

bool snake_is_out_of_bounds(Snake *snake, int32_t width, int32_t height) {
    Position headPosition = snake_head_position(snake);
    return headPosition.x < 0 ||
           headPosition.x >= width ||
           headPosition.y < 0 ||
           headPosition.y >= height;
}

void snake_move_ahead(Snake *snake) {
    Position nextHeadPosition = snake_next_head_position(snake);
    if (snake->head_index == snake->length - 1) {
        snake->head_index = 0;
    } else {
        snake->head_index++;
    }
    snake->segments[snake->head_index] = nextHeadPosition;
}

void snake_grow(Snake *snake) {
    Position nextHeadPosition = snake_next_head_position(snake);
    if (snake->head_index == snake->length) {
        snake->segments[snake->length] = nextHeadPosition;
    } else {
        for (int i = snake->length; i > snake->head_index; i--) {
            snake->segments[i + 1] = snake->segments[i];
        }
        snake->segments[snake->head_index + 1] = nextHeadPosition;
    }
    snake->length++;
}

void paint_background(void) {
    canvas_set_fill_style(COLOR_BACKGROUND);
    canvas_fill_rect(0, 0, GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE);
}

void paint_snake(Snake *snake) {
    canvas_set_fill_style(COLOR_SNAKE);
    for (int i = 0; i < snake->length; i++) {
        Position segment = snake->segments[i];
        canvas_fill_rect(segment.x * CELL_SIZE, segment.y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
    }
}

void paint_apple(Position apple) {
    canvas_set_fill_style(COLOR_APPLE);
    canvas_fill_rect(apple.x * CELL_SIZE, apple.y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
}

typedef struct {
    Snake snake;
    Position apple;
    int32_t stepPeriod;
    int32_t score;
    int32_t next_reward;
} GameState;

static GameState GAME_STATE;


void change_snake_direction(GameState *state, Direction d) {
    if (direction_is_opposite(state->snake.direction, d)) {
        return;
    }
    state->snake.direction = d;
}

void on_key_down(KeyCode code) {
    switch (code) {
    case KEY_CODE_ARROW_UP:
        change_snake_direction(&GAME_STATE, DIRECTION_UP);
        break;
    case KEY_CODE_ARROW_DOWN:
        change_snake_direction(&GAME_STATE, DIRECTION_DOWN);
        break;
    case KEY_CODE_ARROW_LEFT:
        change_snake_direction(&GAME_STATE, DIRECTION_LEFT);
        break;
    case KEY_CODE_ARROW_RIGHT:
        change_snake_direction(&GAME_STATE, DIRECTION_RIGHT);
        break;
    }
}

void speedup_game(GameState *state) {
    if (state->stepPeriod > 50) {
        state->stepPeriod -= 25;
        snake_step_period_updated(state->stepPeriod);
    }
}

bool snake_will_eat_apple(GameState *state) { 
    return position_equals(snake_next_head_position(&state->snake), state->apple);
}

void update_score(GameState *state) {
    state->score += state->next_reward;
    state->next_reward += 10;
}

void teleport_apple(GameState *state) {
    state->apple.x = js_random(GRID_WIDTH);
    state->apple.y = js_random(GRID_HEIGHT);
}

void repaint(GameState *state) {
    paint_background();
    paint_snake(&state->snake);
    paint_apple(state->apple);
    canvas_fill();
}

void step(int32_t timestamp) {
    if (snake_will_eat_apple(&GAME_STATE)) {
        snake_grow(&GAME_STATE.snake);
        teleport_apple(&GAME_STATE);
        speedup_game(&GAME_STATE);
        update_score(&GAME_STATE);
        snake_score_changed(GAME_STATE.score);
    } else {
        snake_move_ahead(&GAME_STATE.snake);
    }
    if (snake_is_out_of_bounds(&GAME_STATE.snake, GRID_WIDTH, GRID_HEIGHT) || snake_eats_himself(&GAME_STATE.snake)) {
        snake_game_over();
    }
    repaint(&GAME_STATE);
}

void init() {
    GAME_STATE.stepPeriod = 300;
    GAME_STATE.next_reward = 10;
    teleport_apple(&GAME_STATE);
    GAME_STATE.snake.length = 4;
    GAME_STATE.snake.head_index = 3;
    GAME_STATE.snake.direction = DIRECTION_RIGHT;
    GAME_STATE.snake.segments[1].x = 1;
    GAME_STATE.snake.segments[2].x = 2;
    GAME_STATE.snake.segments[3].x = 3;
    repaint(&GAME_STATE);
    snake_score_changed(0);
}