/*Copyrights @ Ahmed Soliman www.ahmedesoliman.com*/
// predict.cpp

#include "predict.h"

//defualt constructor
Predict::Predict()
{
}

//default destructor
Predict::~Predict()
{

    // Delete capture object
    destroyAllWindows(); // destroy the all open windows
    capture.release();   // Delete capture object
}

void Predict::load_ASL()
{
    //*** Preload letter train images starts ***//
    for (int i = 0; i < MAX_LETTERS; i++)
    {
        char buffer[13 * sizeof(char)];

        sprintf_s(buffer, "train/%c.png", ('a' + i)); // foramting

        Mat img1 = imread(buffer, 1);

        if (img1.data)
        {
            Mat img2, threshold_output;

            cvtColor(img1, img2, COLOR_RGB2GRAY);

            // Detect edges using Threshold
            //The threshold method returns two outputs. The first is the threshold that was used and the second output is the thresholded image.
            threshold(img2, threshold_output, THRESH, 255, THRESH_BINARY);

            //findcontours() function retrieves contours from the binary image using the openCV algorithm[193].
            //The contours are a useful tool for shape analysisand object and detectionand recognition.
            findContours(threshold_output, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

            letters[i] = contours[0];
            //contours returns a vector<vector<point>>
        }
    }
    //***Preload letter train images ends***//

    //*** learn starts ***//

    backGroundMOG2 = createBackgroundSubtractorMOG2(10000, 200, false);

    //***learn ends  ***//

} /* end of asl_init()*/

void Predict::predictApp(char key)
{

    capture = VideoCapture(0);

    //Creates MOG2 Background Subtractor.
    backGroundMOG2 = createBackgroundSubtractorMOG2(10000, 200, false);

    while (key != KEY_ESC)
    {
        /*    cout << "inside training \n";*/
        try
        {
            if (!capture.isOpened())
            {
                // Error in opening the video input
                cout << "Cannot Open Webcam... " << endl;
                exit(1);
                throw "Cannot Open Webcam... \n";
            }
        }

        catch (string str)
        {
            cout << str;
        }
        try
        {
            if (!capture.read(frame))
            {
                cout << "Unable to read next frame..." << endl;
                cout << "Exiting..." << endl;
                exit(1);
                throw " Unable to read next frame...  \n";
            }
        }
        catch (string str)
        {
            cout << str;
        }

        // Crop Frame to smaller region using the rectangle of interest method
        Rect myROI(200, 200, 200, 200);

        Mat cropFrame = frame(myROI);

        // Update the background model
        backGroundMOG2->apply(cropFrame, fgMaskMOG2, 0);

        // Detect edges using Threshold
        /*adaptiveThreshold(fgMaskMOG2, threshold_output, THRESH, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 3, 0);  -- adabtiveThreshhold can be another method to be used*/
        threshold(fgMaskMOG2, threshold_output, THRESH, 255, THRESH_BINARY);

        // Find contours
        findContours(threshold_output, feature_image, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

        // Find largest contour

        Mat drawing1 = Mat::zeros(cropFrame.size(), CV_8UC3); // empty matrix to draw the contours on it - intialize it to Mat::zeros means all black
        double largest_area = 0;

        for (int j = 0; j < feature_image.size(); j++)
        {
            double area = contourArea(feature_image[j], false); // Find the area of the contour
            if (area > largest_area)
            {
                largest_area = area;
                maxIndex = j; // Store the index of largest contour
            }
        }

        // Draw Largest Contours
        drawContours(drawing1, feature_image, maxIndex, WHITE, FILLED); // fill white

        // Draw Contours
        Mat contourImg = Mat::zeros(cropFrame.size(), CV_8UC3);
        drawContours(contourImg, feature_image, maxIndex, RED, 2, 8, hierarchy, 0, Point(0, 0));

        //Reset if too much noise
        //    Scalar sums = sum(drawing1);
        //int s = sums[0] + sums[1] + sums[2] + sums[3];
        //if (s >= RESET_THRESH)
        //{
        //    backGroundMOG2 = createBackgroundSubtractorMOG2(10000, 200, false);
        //    continue;
        //}

        // Show the current frame and the foreground masks
        imshow("Crop Frame", cropFrame);
        imshow("Mask", drawing1);
        imshow("Foregound Mask", fgMaskMOG2);
        imshow("Contour image", contourImg);

        //if (contourImg.rows > 0)
        //    imshow("Contour", contourImg);

        key = waitKey(1);

        // Manual reset the keyboard
        if (key == ' ')
            backGroundMOG2 = createBackgroundSubtractorMOG2(10000, 200, false);

        //f3_identify_letter
        if (feature_image.size() > 0 && frames++ > SAMPLE_RATE && feature_image[maxIndex].size() >= 5)
        {
            RotatedRect testRect = fitEllipse(feature_image[maxIndex]);
            //fits an ellipse around a set of 2d points. The function calculates the ellipse that fits(in a least-sense) a set of 2D points best of all.
            //it returns the rotated rectangle in which the ellipse is inscribed. the first algorithm - Param(points input 2d point set, stored in std::vector<> or Mat)

            frames = 0;

            double lowestDiff = HUGE_VAL;

            for (int i = 0; i < MAX_LETTERS; i++)
            {
                if (letters[i].size() == 0)
                    continue;

                double difference = distance(letters[i], feature_image[maxIndex]);

                if (difference < lowestDiff)
                {
                    lowestDiff = difference;
                    asl_letter = 'a' + i;
                }
            }

            if (lowestDiff > DIFF_THRESH)
            { // Dust
                asl_letter = 0;
            }
            ofstream myfile;
            
            if (isalpha(asl_letter)) {
                myfile.open("C:\\Unity_Projects\\Sign_AI_VR_Game\\Sign_AI_VR_Game_V.1.0\\Assets\\StreamingAssets\\RecallText\\Alphabets.txt", ios::out | ios::app);
                myfile << asl_letter;
                cout << "The letter is: " << asl_letter << " | difference: " << lowestDiff << endl;
                /*   cout << "Writing the letter: " << asl_letter << " -> to a file.\n";*/
            }
           
            myfile.close();

            displayLetter();
        }
    }
}

void Predict::displayLetter()
{

    int letterCount = 0; // number of letters captured since last displayletterCount = 0;
    char lastLetters[NUM_LAST_LETTERS] = { 0 };

    //creates a Mat object filled with zeros
    Mat letter_image = Mat::zeros(200, 200, CV_8UC3);
    char lastExecLetter = 0; // last letter sent

    letterCount %= NUM_LAST_LETTERS;         // Show majority of last letters captured
    lastLetters[letterCount++] = asl_letter; // input from f4
    letter_image = Mat::zeros(200, 200, CV_8UC3);

    int counts[MAX_LETTERS + 1] = { 0 };

    for (int i = 0; i < NUM_LAST_LETTERS; i++)
        counts[lastLetters[i] + 1 - 'a']++;

    int maxCount = 0;
    char maxChar = 0;
    for (int i = 0; i < MAX_LETTERS + 1; i++)
    {
        if (counts[i] > maxCount)
        {
            maxCount = counts[i];
            maxChar = i;
        }
    }

    if (maxChar && maxCount >= MIN_FREQ)
    {
        maxChar = maxChar - 1 + 'a';
        char buffer[2 * sizeof(char)];
        sprintf_s(buffer, "%c", maxChar); // foramting

        putText(letter_image, buffer, Point(10, 75), FONT_HERSHEY_SIMPLEX, 12, WHITE, 1, 1); // put the text on the this point

        vector<vector<Point>> dummy;

        dummy.push_back(letters[maxChar - 'a']);

        drawContours(letter_image, dummy, maxIndex, BLUE, 2, 8, display, 0, Point(0, 0));
        if (maxChar != lastExecLetter)
        {
            lastExecLetter = maxChar;
        }
    }

    imshow("Letter", letter_image); // output f5--> letter_image
    char q = waitKey(33);
}