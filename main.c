#include "raylib.h"
#include <math.h>

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 600

#define BALL_RADIUS 9.0f
#define PLAY_TOP 70
#define PLAY_BOTTOM (SCREEN_HEIGHT - 38)

// Reset the ball after starting or losing a life.
void ResetBall(Vector2 *ball, Vector2 *speed)
{
    ball->x = 105.0f;
    ball->y = SCREEN_HEIGHT / 2.0f;

    speed->x = 320.0f;
    speed->y = -150.0f;
}

int main(void)
{
    InitWindow(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        "Pong Solo - C and raylib"
    );

    SetTargetFPS(60);

    // The player's paddle.
    Rectangle paddle = {
        42.0f,
        SCREEN_HEIGHT / 2.0f - 50.0f,
        18.0f,
        100.0f
    };

    // The ball's position and speed.
    Vector2 ball;
    Vector2 ballSpeed;

    ResetBall(&ball, &ballSpeed);

    int score = 0;
    int lives = 3;
    bool gameOver = false;

    // Main game loop.
    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        // Limit large movement jumps if a frame stalls.
        if (deltaTime > 0.033f)
        {
            deltaTime = 0.033f;
        }

        // Restart the game with R.
        if (IsKeyPressed(KEY_R))
        {
            score = 0;
            lives = 3;
            gameOver = false;

            paddle.y = SCREEN_HEIGHT / 2.0f - 50.0f;
            ResetBall(&ball, &ballSpeed);
        }

        // Update movement while the game is active.
        if (!gameOver)
        {
            float paddleSpeed = 420.0f;

            // Move using W/S or the arrow keys.
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
            {
                paddle.y -= paddleSpeed * deltaTime;
            }

            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
            {
                paddle.y += paddleSpeed * deltaTime;
            }

            // Keep the paddle inside the playing area.
            if (paddle.y < PLAY_TOP)
            {
                paddle.y = PLAY_TOP;
            }

            if (paddle.y + paddle.height > PLAY_BOTTOM)
            {
                paddle.y = PLAY_BOTTOM - paddle.height;
            }

            // Move the ball.
            ball.x += ballSpeed.x * deltaTime;
            ball.y += ballSpeed.y * deltaTime;

            // Bounce off the top wall.
            if (ball.y - BALL_RADIUS < PLAY_TOP)
            {
                ball.y = PLAY_TOP + BALL_RADIUS;
                ballSpeed.y = fabsf(ballSpeed.y);
            }

            // Bounce off the bottom wall.
            if (ball.y + BALL_RADIUS > PLAY_BOTTOM)
            {
                ball.y = PLAY_BOTTOM - BALL_RADIUS;
                ballSpeed.y = -fabsf(ballSpeed.y);
            }

            // The right wall gives the player a point.
            if (ball.x + BALL_RADIUS >= SCREEN_WIDTH)
            {
                ball.x = SCREEN_WIDTH - BALL_RADIUS;
                ballSpeed.x = -fabsf(ballSpeed.x);
                score++;
            }

            // Bounce off the player's paddle.
            if (ballSpeed.x < 0.0f &&
                CheckCollisionCircleRec(ball, BALL_RADIUS, paddle))
            {
                ball.x = paddle.x + paddle.width + BALL_RADIUS;
                ballSpeed.x = fabsf(ballSpeed.x);

                // Change the ball's angle depending on
                // where it hits the paddle.
                float hitPosition =
                    (ball.y - (paddle.y + paddle.height / 2.0f))
                    / (paddle.height / 2.0f);

                if (hitPosition < -1.0f)
                    hitPosition = -1.0f;

                if (hitPosition > 1.0f)
                    hitPosition = 1.0f;

                ballSpeed.y = hitPosition * 260.0f;

                // Avoid an almost perfectly horizontal bounce.
                if (fabsf(ballSpeed.y) < 80.0f)
                {
                    ballSpeed.y =
                        hitPosition < 0.0f ? -80.0f : 80.0f;
                }
            }

            // Missing the ball costs one life.
            if (ball.x + BALL_RADIUS < 0.0f)
            {
                lives--;

                if (lives <= 0)
                {
                    gameOver = true;
                }
                else
                {
                    ResetBall(&ball, &ballSpeed);
                }
            }
        }

        // Draw everything.
        BeginDrawing();

        ClearBackground((Color){ 9, 13, 26, 255 });

        DrawText(
            TextFormat("SCORE %03i", score),
            24, 18, 20, SKYBLUE
        );

        DrawText(
            "PONG SOLO",
            SCREEN_WIDTH / 2 - MeasureText("PONG SOLO", 25) / 2,
            15, 25, RAYWHITE
        );

        DrawText(
            TextFormat("LIVES %i", lives),
            SCREEN_WIDTH - 105, 18, 20, SKYBLUE
        );

        DrawLine(
            0, 60, SCREEN_WIDTH, 60,
            (Color){ 45, 60, 85, 255 }
        );

        // Dashed centre line for the arcade look.
        for (int y = PLAY_TOP; y < PLAY_BOTTOM; y += 24)
        {
            DrawRectangle(
                SCREEN_WIDTH / 2 - 1,
                y,
                2, 10,
                (Color){ 35, 53, 76, 255 }
            );
        }

        DrawRectangleRec(paddle, SKYBLUE);

        DrawCircleV(
            ball, BALL_RADIUS + 4.0f,
            (Color){ 65, 190, 255, 55 }
        );

        DrawCircleV(ball, BALL_RADIUS, RAYWHITE);

        DrawText(
            "W/S OR ARROWS: MOVE    R: RESTART    ESC: QUIT",
            20, SCREEN_HEIGHT - 22, 14, GRAY
        );

        if (gameOver)
        {
            DrawRectangle(
                0, PLAY_TOP,
                SCREEN_WIDTH, PLAY_BOTTOM - PLAY_TOP,
                (Color){ 4, 7, 14, 220 }
            );

            DrawText(
                "GAME OVER",
                SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 44) / 2,
                245, 44, RED
            );

            DrawText(
                "Press R to play again",
                SCREEN_WIDTH / 2 -
                    MeasureText("Press R to play again", 22) / 2,
                305, 22, RAYWHITE
            );
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
