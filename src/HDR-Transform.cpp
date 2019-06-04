#include <piccante.hpp>

void checkWritingValid(bool flag)
{
	if (flag) {
		printf(" -- Result: Ok\n");
	}
	else {
		printf(" -- Result: Writing had some issues!\n");
	}
}


int main(int argc, char *argv[])
{
    std::string img_str;
	int hdr_mode;

	if (argc < 3) {
		printf("[Error] HDR+ Transformer requires at least two parameters!\n[Info]  Usage: ./HDR-Transformer Image-Path HDR-Mode(Range: [0, 4])\n");
		printf("[Info]  HDR-Mode Execution Speed Rank: 3 > 1 > 4 > 0 > 2");
		return -1;
	}
	else {
		img_str = argv[1];
		hdr_mode = atoi(argv[2]);

	}

    printf("Reading Target Image: %s\n", img_str.c_str());

    pic::Image img;
    img.Read(img_str);

    if(img.isValid()) {
        std::string nameOut = pic::getFileNameOnly(img_str);

        bool bWritten;
        pic::Image *image_tmo = NULL;

		switch (hdr_mode) 
		{
			case 0:
				printf("Tone mapping mode: Schlick TMO");
				image_tmo = pic::SchlickTMO::execute(&img, image_tmo);

				bWritten = image_tmo->Write("../resources/output/" + nameOut + "_schlick_tmo.png", pic::LT_NOR_GAMMA);

				checkWritingValid(bWritten);

				break;

			case 1:
				printf("Tone mapping mode: Global Reinhard TMO");
				image_tmo = pic::ReinhardTMO::executeGlobal1(&img, image_tmo);

				bWritten = image_tmo->Write("../resources/output/" + nameOut + "_reinhard_gtmo.png", pic::LT_NOR_GAMMA);

				checkWritingValid(bWritten);

				break;

			case 2:
				printf("Tone mapping mode: Local Reinhard TMO");
				image_tmo = pic::ReinhardTMO::executeLocal1(&img, image_tmo);

				bWritten = image_tmo->Write("../resources/output/" + nameOut + "_reinhard_ltmo.png", pic::LT_NOR_GAMMA);

				checkWritingValid(bWritten);

				break;

			case 3:
				printf("Tone mapping mode: Drago TMO");
				image_tmo = pic::DragoTMO::execute(&img, image_tmo);

				bWritten = image_tmo->Write("../resources/output/" + nameOut + "_drago_tmo.png", pic::LT_NOR_GAMMA);

				checkWritingValid(bWritten);

				break;

			case 4:
				printf("Tone mapping mode: Ward Histogram Adjustment TMO");
				image_tmo = pic::WardHistogramTMO::execute(&img, image_tmo);

				bWritten = image_tmo->Write("../resources/output/" + nameOut + "_ward_tmo.png", pic::LT_NOR_GAMMA);

				checkWritingValid(bWritten);
		}

    } else {
        printf("No, the file is not valid!\n");
    }

    return 0;
}