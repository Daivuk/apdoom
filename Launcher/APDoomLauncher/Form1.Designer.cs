
namespace APDoomLauncher
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.label1 = new System.Windows.Forms.Label();
            this.txtWidth = new System.Windows.Forms.TextBox();
            this.txtHeight = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.chkFullscreen = new System.Windows.Forms.CheckBox();
            this.btnLaunchDOOM = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label6 = new System.Windows.Forms.Label();
            this.txtCommandLine = new System.Windows.Forms.TextBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.txtPassword = new System.Windows.Forms.TextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.txtServer = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.txtPlayer = new System.Windows.Forms.TextBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.label7 = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(36, 54);
            this.label1.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(35, 13);
            this.label1.TabIndex = 1;
            this.label1.Text = "Width";
            // 
            // txtWidth
            // 
            this.txtWidth.Location = new System.Drawing.Point(148, 51);
            this.txtWidth.Margin = new System.Windows.Forms.Padding(2);
            this.txtWidth.Name = "txtWidth";
            this.txtWidth.Size = new System.Drawing.Size(76, 20);
            this.txtWidth.TabIndex = 2;
            this.txtWidth.Text = "1280";
            // 
            // txtHeight
            // 
            this.txtHeight.Location = new System.Drawing.Point(148, 74);
            this.txtHeight.Margin = new System.Windows.Forms.Padding(2);
            this.txtHeight.Name = "txtHeight";
            this.txtHeight.Size = new System.Drawing.Size(76, 20);
            this.txtHeight.TabIndex = 3;
            this.txtHeight.Text = "720";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(36, 76);
            this.label2.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(38, 13);
            this.label2.TabIndex = 3;
            this.label2.Text = "Height";
            // 
            // chkFullscreen
            // 
            this.chkFullscreen.AutoSize = true;
            this.chkFullscreen.Location = new System.Drawing.Point(149, 29);
            this.chkFullscreen.Margin = new System.Windows.Forms.Padding(2);
            this.chkFullscreen.Name = "chkFullscreen";
            this.chkFullscreen.Size = new System.Drawing.Size(74, 17);
            this.chkFullscreen.TabIndex = 1;
            this.chkFullscreen.Text = "Fullscreen";
            this.chkFullscreen.UseVisualStyleBackColor = true;
            this.chkFullscreen.CheckedChanged += new System.EventHandler(this.chkFullscreen_CheckedChanged);
            // 
            // btnLaunchDOOM
            // 
            this.btnLaunchDOOM.Location = new System.Drawing.Point(194, 566);
            this.btnLaunchDOOM.Margin = new System.Windows.Forms.Padding(2);
            this.btnLaunchDOOM.Name = "btnLaunchDOOM";
            this.btnLaunchDOOM.Size = new System.Drawing.Size(112, 24);
            this.btnLaunchDOOM.TabIndex = 8;
            this.btnLaunchDOOM.Text = "Launch DOOM";
            this.btnLaunchDOOM.UseVisualStyleBackColor = true;
            this.btnLaunchDOOM.Click += new System.EventHandler(this.btnLaunchDOOM_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.label6);
            this.groupBox1.Controls.Add(this.txtCommandLine);
            this.groupBox1.Controls.Add(this.txtWidth);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Controls.Add(this.chkFullscreen);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.txtHeight);
            this.groupBox1.Location = new System.Drawing.Point(76, 240);
            this.groupBox1.Margin = new System.Windows.Forms.Padding(2);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Padding = new System.Windows.Forms.Padding(2);
            this.groupBox1.Size = new System.Drawing.Size(350, 142);
            this.groupBox1.TabIndex = 7;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Game Settings";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(36, 99);
            this.label6.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(77, 13);
            this.label6.TabIndex = 6;
            this.label6.Text = "Command Line";
            // 
            // txtCommandLine
            // 
            this.txtCommandLine.Location = new System.Drawing.Point(148, 97);
            this.txtCommandLine.Margin = new System.Windows.Forms.Padding(2);
            this.txtCommandLine.Name = "txtCommandLine";
            this.txtCommandLine.Size = new System.Drawing.Size(154, 20);
            this.txtCommandLine.TabIndex = 4;
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.txtPassword);
            this.groupBox2.Controls.Add(this.label5);
            this.groupBox2.Controls.Add(this.txtServer);
            this.groupBox2.Controls.Add(this.label3);
            this.groupBox2.Controls.Add(this.label4);
            this.groupBox2.Controls.Add(this.txtPlayer);
            this.groupBox2.Location = new System.Drawing.Point(76, 412);
            this.groupBox2.Margin = new System.Windows.Forms.Padding(2);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Padding = new System.Windows.Forms.Padding(2);
            this.groupBox2.Size = new System.Drawing.Size(350, 120);
            this.groupBox2.TabIndex = 8;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Archipelago Settings";
            // 
            // txtPassword
            // 
            this.txtPassword.Location = new System.Drawing.Point(149, 77);
            this.txtPassword.Margin = new System.Windows.Forms.Padding(2);
            this.txtPassword.Name = "txtPassword";
            this.txtPassword.PasswordChar = '*';
            this.txtPassword.Size = new System.Drawing.Size(153, 20);
            this.txtPassword.TabIndex = 7;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(36, 80);
            this.label5.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(53, 13);
            this.label5.TabIndex = 5;
            this.label5.Text = "Password";
            // 
            // txtServer
            // 
            this.txtServer.Location = new System.Drawing.Point(149, 32);
            this.txtServer.Margin = new System.Windows.Forms.Padding(2);
            this.txtServer.Name = "txtServer";
            this.txtServer.Size = new System.Drawing.Size(153, 20);
            this.txtServer.TabIndex = 5;
            this.txtServer.Text = "archipelago.gg:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(36, 34);
            this.label3.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(38, 13);
            this.label3.TabIndex = 1;
            this.label3.Text = "Server";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(36, 57);
            this.label4.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(56, 13);
            this.label4.TabIndex = 3;
            this.label4.Text = "Slot Name";
            // 
            // txtPlayer
            // 
            this.txtPlayer.Location = new System.Drawing.Point(149, 54);
            this.txtPlayer.Margin = new System.Windows.Forms.Padding(2);
            this.txtPlayer.Name = "txtPlayer";
            this.txtPlayer.Size = new System.Drawing.Size(153, 20);
            this.txtPlayer.TabIndex = 6;
            // 
            // pictureBox1
            // 
            this.pictureBox1.BackgroundImage = global::APDoomLauncher.Properties.Resources.Title;
            this.pictureBox1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
            this.pictureBox1.Location = new System.Drawing.Point(66, 19);
            this.pictureBox1.Margin = new System.Windows.Forms.Padding(2);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(369, 195);
            this.pictureBox1.TabIndex = 0;
            this.pictureBox1.TabStop = false;
            this.pictureBox1.Paint += new System.Windows.Forms.PaintEventHandler(this.pictureBox1_Paint);
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(9, 596);
            this.label7.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(111, 13);
            this.label7.TabIndex = 9;
            this.label7.Text = "APDOOM 0.3.9 (beta)";
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(503, 617);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.btnLaunchDOOM);
            this.Controls.Add(this.pictureBox1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Margin = new System.Windows.Forms.Padding(2);
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Archipelago DOOM Launcher";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox txtWidth;
        private System.Windows.Forms.TextBox txtHeight;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.CheckBox chkFullscreen;
        private System.Windows.Forms.Button btnLaunchDOOM;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.TextBox txtPassword;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox txtServer;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox txtPlayer;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.TextBox txtCommandLine;
        private System.Windows.Forms.Label label7;
    }
}

