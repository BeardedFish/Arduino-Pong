#include <TFT.h>
#include <SPI.h>
#include <IRremote.h>

#define IR_RECEIVER_PIN 2
#define SPEAKER_OUTPUT_PIN 3
#define TFT_CS_PIN   10
#define TFT_DC_PIN   9
#define TFT_RST_PIN  8

TFT tft = TFT(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

IRrecv irrecv(IR_RECEIVER_PIN);
decode_results results;

const int PADDLE_WIDTH = 25; // The width of the paddle.
const int PADDLE_HEIGHT = 5; // The height of the paddle.
const int BALL_DIMENSIONS = 3; // The length/width of the ball.
const int MAX_SCORE_TO_WIN = 5; // The maximum score a player needs until it's game over.

int ballDirectionX = 1;
int ballDirectionY = 1;

int paddleX = 0;
int paddleY = 0;
int oldPaddleX, oldPaddleY;

int opponentX = 0;
int opponentY = 0;
int oldOpponentX, oldOpponentY;

int ballX, ballY, oldBallX, oldBallY;
int playerScore, opponentScore;

boolean gameOver = false;
boolean gameOverScreenDrawn = false;
boolean opponentWon = false;

boolean leftDown = false;
boolean rightDown = false;

char printout[4];

void setup() 
{ 
  Serial.begin(9600);

  pinMode(SPEAKER_OUTPUT_PIN, OUTPUT);

  irrecv.enableIRIn();

  // Setup the TFT LCD display:
  tft.begin();
  tft.setRotation(2);
  tft.background(0, 0, 0);
  
  opponentY = tft.height() - 5;
  
  centerPaddles();
  centerBall();
}

void loop() 
{
  if (gameOver)
  {
    drawGameOverScreen();
    return;
  }

  checkScores(); 
  drawPaddles();
  drawCourt();
  drawScores();
  moveOpponentPaddle();
  moveBall();
  receiveAndProcessIRCommands();
}

void drawGameOverScreen()
{
  if (!gameOverScreenDrawn) // We don't want to always draw the game over screen because that will cause flickering...
  {
    tft.fillScreen(0x0000);
  
    tft.setTextSize(2);
    tft.stroke(255, 255, 255);
    
    if (opponentWon)
    {
      tft.text("YOU LOST!", tft.width() / 2 - 50, tft.height() / 2);
      
      delay(1000);

      // Play the "you lost" tune:
      tone(SPEAKER_OUTPUT_PIN, 3000);
      delay(100);
      noTone(SPEAKER_OUTPUT_PIN);
      tone(SPEAKER_OUTPUT_PIN, 2000);
      delay(100);
      noTone(SPEAKER_OUTPUT_PIN);
      tone(SPEAKER_OUTPUT_PIN, 1000);
      delay(100);
      noTone(SPEAKER_OUTPUT_PIN);
    }
    else
    {
      tft.text("YOU WON!", tft.width() / 2 - 45, tft.height() / 2);

      delay(1000);
      
      // Play the "you won" tune:
      tone(SPEAKER_OUTPUT_PIN, 10000);
      delay(125);
      noTone(SPEAKER_OUTPUT_PIN);
      tone(SPEAKER_OUTPUT_PIN, 13000);
      delay(125);
      noTone(SPEAKER_OUTPUT_PIN);
      tone(SPEAKER_OUTPUT_PIN, 16000);
      delay(125);
      noTone(SPEAKER_OUTPUT_PIN);
      tone(SPEAKER_OUTPUT_PIN, 19000);
      delay(125);
      noTone(SPEAKER_OUTPUT_PIN);
    }
    
    gameOverScreenDrawn = true;
  }
}

/*
 * Checks to see if someone won the game, causing a game over.
 */
void checkScores()
{ 
  if (playerScore >= MAX_SCORE_TO_WIN)
  {
      gameOver = true;
      opponentWon = false;
  }

  if (opponentScore >= MAX_SCORE_TO_WIN)
  {
      gameOver = true;
      opponentWon = true;
  }
}

/*
 * Draws the entire court for the game on the display.
 */
void drawCourt()
{
  int width = tft.width();
  int height = tft.height();

  // Draw the borders on the screen: 
  tft.fill(255, 255, 255);
  tft.rect(0, 0, width, 1);
  tft.rect(0, 0, 1, height);
  tft.rect(width - 1, 0, 1, height);
  tft.rect(0, height - 1, width, 1);

  // Draw the diving line between the two players:
  tft.rect(0, height / 2, width, 1);
}

/*
 * Draws both the players score and the opponents score on the display.
 */
void drawScores()
{
  int width = tft.width();
  int height = tft.height();
  String score = String(playerScore);

  // Draw the opponents score onto the screebL
  score.toCharArray(printout,4);
  tft.setTextSize(0.5);
  tft.stroke(255, 255, 255);
  tft.text(printout, width / 2, height / 2 - 12);

  score = String(opponentScore);
  score.toCharArray(printout,4);
  tft.text(printout, width / 2, height / 2 + 8);

  tft.noStroke();
}

/*
 * Draws both the players paddle and the opponents paddle on the display.
 */
void drawPaddles()
{
  // Erease the old positions of the both the player and the opponents paddle on the screen:
  tft.fill(0, 0, 0);
  if (oldPaddleX != paddleX || oldPaddleY != paddleY) 
  {
    tft.rect(oldPaddleX, oldPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT);
  }

  if (oldOpponentX != opponentX || oldOpponentY != opponentY) 
  {
    tft.rect(oldOpponentX, oldOpponentY, PADDLE_WIDTH, PADDLE_HEIGHT);
  }

  // Draw player paddle:
  tft.fill(255, 255, 255);
  tft.rect(paddleX, paddleY, PADDLE_WIDTH, PADDLE_HEIGHT);
  oldPaddleX = paddleX;
  oldPaddleY = paddleY;

  // Draw the opponents paddle:
  tft.rect(opponentX, opponentY, PADDLE_WIDTH, PADDLE_HEIGHT);
  oldOpponentX = opponentX;
  oldOpponentY = opponentY;
}

/*
 * Moves the ball depending on it's
 */
void moveBall() 
{
  // Move the ball according to its respective x and y velocity:
  ballX += ballDirectionX;
  ballY += ballDirectionY;

  // CHECK COLLISIONS:
  
  // Ball hit the right wall:
  if (ballX + BALL_DIMENSIONS >= tft.width()) 
  {
    ballX = tft.width() - BALL_DIMENSIONS;
    ballDirectionX = -ballDirectionX;
    tone(SPEAKER_OUTPUT_PIN, 5000);
    delay(75);
    noTone(SPEAKER_OUTPUT_PIN);
  }

  // Ball hit the left wall:
  if (ballX <= 0)
  {
    ballX = 0;
    ballDirectionX = -ballDirectionX;
    tone(SPEAKER_OUTPUT_PIN, 5000);
    delay(75);
    noTone(SPEAKER_OUTPUT_PIN);
  }

  // Ball hit the bottom wall:
  if (ballY + BALL_DIMENSIONS >= tft.height()) 
  {
    ballDirectionY = -ballDirectionY;

    score(false);
  }

  // Ball hit the top wall:
  if (ballY <= 0)
  {
      score(true);
  }

  // Check if the ball intersects with either the player paddle or opponent paddle. If it does, then bounce the ball back.
  if (intersects(ballX, ballY, paddleX, paddleY, PADDLE_WIDTH, PADDLE_HEIGHT)) 
  {
    ballY = paddleY + BALL_DIMENSIONS * 2;
    ballDirectionY = -ballDirectionY;

    tone(SPEAKER_OUTPUT_PIN, 5000);
    delay(75);
    noTone(SPEAKER_OUTPUT_PIN);
  }

  if (intersects(ballX, ballY, opponentX, opponentY, PADDLE_WIDTH, PADDLE_HEIGHT)) 
  {
    ballY = opponentY - BALL_DIMENSIONS * 2;
    ballDirectionY = -ballDirectionY;

    tone(SPEAKER_OUTPUT_PIN, 5000);
    delay(75);
    noTone(SPEAKER_OUTPUT_PIN);
  }

  // Erase the ball's old location:
  tft.fill(0, 0, 0);
  if (oldBallX != ballX || oldBallY != ballY) 
  {
    tft.rect(oldBallX, oldBallY, BALL_DIMENSIONS, BALL_DIMENSIONS);
  }

  // Draw the balls new location:
  tft.fill(255, 255, 255);
  tft.rect(ballX, ballY, BALL_DIMENSIONS, BALL_DIMENSIONS);
  oldBallX = ballX;
  oldBallY = ballY;
}

/*
 * Centers the paddles X locations on the display.
 */
void centerPaddles()
{
  paddleX = (tft.width() / 2) - 15;
  opponentX = (tft.width() / 2) - 15;
}

/*
 * Centers the ball on the display.
 */
void centerBall()
{
  ballX = (tft.width() / 2) - BALL_DIMENSIONS;
  ballY = (tft.height() / 2) - BALL_DIMENSIONS;
}

/*
 * Moves the opponents paddle.
 */
void moveOpponentPaddle()
{
    if (ballY <= tft.height() / 2 - random(-5, 10))
    {
      return;
    }
    
    if (millis() % 5 == 0)
    {
      int moveSpeed = random(2, 3);
      
      if (ballX < opponentX + 10)
      {
          opponentX -= moveSpeed;

          if (opponentX < 0)
          {
              opponentX = 0;
          }
      }
      else
      {
          opponentX += moveSpeed;
          
          if (opponentX > tft.width())
          {
              opponentX = tft.width() - 20;
          }
      }
    }
}

/*
 * Adds 1 point to the players score depending on who scored, shows a screen that says "SCORED!", and starts a new round.
 */
void score(boolean opponentScored)
{ 
  // Add 1 point to the player who scored:
  if (opponentScored)
  {
    opponentScore++;
  }
  else
  {
    playerScore++;
  }

  // Draw a blank screen with "SCORE!" on the display:
  tft.fillScreen(0x0000);
  tft.setTextSize(2);
  tft.stroke(255, 255, 255);
  tft.text("SCORE!", tft.width() / 2 - 35, tft.height() / 2);

  delay(250);

  // Play the "score" tune:
  tone(SPEAKER_OUTPUT_PIN, 10000);
  delay(50);
  noTone(SPEAKER_OUTPUT_PIN);
  tone(SPEAKER_OUTPUT_PIN, 11000);
  delay(50);
  noTone(SPEAKER_OUTPUT_PIN);
  tone(SPEAKER_OUTPUT_PIN, 12000);
  delay(50);
  noTone(SPEAKER_OUTPUT_PIN);
  
  delay(1500);
  
  leftDown = false;
  rightDown = false;
  centerPaddles();
  centerBall(); 
  tft.noStroke();
  tft.fillScreen(0x0000);
}

/*
 * Determines whether a rectangleintersects with a rectangle.
 */
boolean intersects(int x, int y, int rectX, int rectY, int rectWidth, int rectHeight) 
{
  boolean result = false;

  if ((x >= rectX && x <= (rectX + rectWidth)) && (y >= rectY && y <= (rectY + rectHeight))) 
  {
    result = true;
  }

  return result;
}

/*
 * Receives IR commands from the receiver and processes them.
 */
void receiveAndProcessIRCommands()
{
    if (irrecv.decode(&results)) 
    {
        switch(results.value)
        {
            // Left button:
            case 0xDEB92:
              if (paddleX <= 0)
              {
                  paddleX = 0;
                  break;
              }
                  
              paddleX -= 4;
              rightDown = false;
              leftDown = true;
              break;
              
            // Right button:
            case 0x3EB92:
              if (paddleX + PADDLE_WIDTH >= tft.width())
              {
                  paddleX = tft.width() - PADDLE_WIDTH;
                  break;
              }
            
              paddleX += 4;
              leftDown = false;
              rightDown = true;
              break;
              
            // Button being held down:
            case 0xFFFFFFFF:
              if (leftDown)
              {
                if (paddleX <= 0)
                {
                    paddleX = 0;
                }
                
                paddleX -=4;
              }

              if (rightDown)
              {
                if (paddleX + PADDLE_WIDTH >= tft.width())
                {
                    paddleX = tft.width() - PADDLE_WIDTH;
                }
                
                paddleX += 4;
              } 
            break;
            
            // Unknown button pressed:
            default: 
              leftDown = false;
              rightDown = false;
              // Serial.print("Unknown button pressed!");
              // Serial.println(results.value, HEX);
              break;
        }

        irrecv.resume();    
    }
}
